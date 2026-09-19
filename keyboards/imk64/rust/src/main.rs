#![no_std]
#![no_main]

// embassy-ch58x-rs's exported interrupt macro retains the runtime's historical
// crate identifier. Keep that compatibility name local while making the
// rusted-ch5 package identity explicit in Cargo and at the entry point.
extern crate qingke_rs as qingke_rt;

use core::ptr;

use embassy_ch58x::gpio::{AnyPin, Drive, Input, Level, Output, Pins, Pull};
use embassy_futures::join::join3;
use embassy_sync::blocking_mutex::raw::CriticalSectionRawMutex;
use embassy_sync::channel::{Channel, TrySendError};
use embassy_time::{Instant, Timer};
use embassy_usb::class::hid::{
    HidBootProtocol, HidProtocolMode, HidReader, HidReaderWriter, HidSubclass, HidWriter, ReportId,
    RequestHandler, State as HidState,
};
use embassy_usb::control::OutResponse;
use embassy_usb::{Builder, Config, Handler};
use embedded_hal::digital::{InputPin, OutputPin};
use portable_atomic::{AtomicBool, AtomicU8, AtomicU16, AtomicU32, Ordering};
use static_cell::StaticCell;

embassy_ch58x::bind_interrupts!(struct Irqs {
    USB => embassy_ch58x::usb::InterruptHandler;
});

const KEYBOARD_ENDPOINT: u8 = 1;
const MATRIX_ROW_COUNT: usize = 5;
const MATRIX_COL_COUNT: usize = 14;

const KEYBOARD_REPORT_DESCRIPTOR: &[u8] = &[
    0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0x05, 0x07, 0x19, 0xe0, 0x29, 0xe7, 0x15, 0x00, 0x25, 0x01,
    0x75, 0x01, 0x95, 0x08, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x81, 0x01, 0x95, 0x06, 0x75, 0x08,
    0x15, 0x00, 0x25, 0x73, 0x19, 0x00, 0x29, 0x73, 0x81, 0x00, 0x05, 0x08, 0x19, 0x01, 0x29, 0x05,
    0x95, 0x05, 0x75, 0x01, 0x91, 0x02, 0x95, 0x01, 0x75, 0x03, 0x91, 0x01, 0xc0,
];

static REPORTS: Channel<CriticalSectionRawMutex, [u8; 8], 32> = Channel::new();
static USB_STARTED: AtomicBool = AtomicBool::new(false);
static USB_CONFIGURED: AtomicBool = AtomicBool::new(false);
static KEYBOARD_LEDS: AtomicU8 = AtomicU8::new(0);
static KEYBOARD_PROTOCOL: AtomicU8 = AtomicU8::new(HidProtocolMode::Report as u8);
static KEYBOARD_IDLE_MS: AtomicU32 = AtomicU32::new(u32::MAX);
static MATRIX_ROWS: [AtomicU16; MATRIX_ROW_COUNT] = [const { AtomicU16::new(0) }; MATRIX_ROW_COUNT];
static MATRIX_GENERATION: AtomicU32 = AtomicU32::new(0);
static MATRIX_READY: AtomicBool = AtomicBool::new(false);

struct Imk64Matrix {
    rows: [Output<AnyPin>; MATRIX_ROW_COUNT],
    cols: [Input<AnyPin>; MATRIX_COL_COUNT],
}

#[repr(C)]
struct Diagnostics {
    magic: u32,
    queued: AtomicU32,
    completed: AtomicU32,
    queue_full: AtomicU32,
    output_reports: AtomicU32,
    qmk_ticks: AtomicU32,
    matrix_scans: AtomicU32,
    invalid_abi_calls: AtomicU32,
    synchronous_wait_requests: AtomicU32,
}

#[used]
#[unsafe(export_name = "CH582_RUST_QMK_DIAGNOSTICS")]
static DIAGNOSTICS: Diagnostics = Diagnostics {
    magic: u32::from_le_bytes(*b"RQK1"),
    queued: AtomicU32::new(0),
    completed: AtomicU32::new(0),
    queue_full: AtomicU32::new(0),
    output_reports: AtomicU32::new(0),
    qmk_ticks: AtomicU32::new(0),
    matrix_scans: AtomicU32::new(0),
    invalid_abi_calls: AtomicU32::new(0),
    synchronous_wait_requests: AtomicU32::new(0),
};

unsafe extern "C" {
    fn platform_setup();
    fn protocol_setup();
    fn ch582_imk64_matrix_backend();
    fn keyboard_setup();
    fn protocol_pre_init();
    fn keyboard_init();
    fn protocol_post_init();
    fn protocol_pre_task();
    fn keyboard_task();
    fn protocol_post_task();
    fn housekeeping_task();
}

struct UsbStateHandler;

impl Handler for UsbStateHandler {
    fn reset(&mut self) {
        USB_CONFIGURED.store(false, Ordering::Release);
        KEYBOARD_PROTOCOL.store(HidProtocolMode::Report as u8, Ordering::Release);
        KEYBOARD_IDLE_MS.store(u32::MAX, Ordering::Release);
    }

    fn configured(&mut self, configured: bool) {
        USB_CONFIGURED.store(configured, Ordering::Release);
    }
}

struct KeyboardRequestHandler;

impl RequestHandler for KeyboardRequestHandler {
    fn get_report(&mut self, id: ReportId, buf: &mut [u8]) -> Option<usize> {
        if id == ReportId::Out(0) && !buf.is_empty() {
            buf[0] = KEYBOARD_LEDS.load(Ordering::Acquire);
            Some(1)
        } else {
            None
        }
    }

    fn set_report(&mut self, id: ReportId, data: &[u8]) -> OutResponse {
        if id == ReportId::Out(0) && data.len() == 1 {
            KEYBOARD_LEDS.store(data[0], Ordering::Release);
            DIAGNOSTICS.output_reports.fetch_add(1, Ordering::Relaxed);
            OutResponse::Accepted
        } else {
            DIAGNOSTICS
                .invalid_abi_calls
                .fetch_add(1, Ordering::Relaxed);
            OutResponse::Rejected
        }
    }

    fn get_protocol(&self) -> HidProtocolMode {
        HidProtocolMode::from(KEYBOARD_PROTOCOL.load(Ordering::Acquire))
    }

    fn set_protocol(&mut self, protocol: HidProtocolMode) -> OutResponse {
        KEYBOARD_PROTOCOL.store(protocol as u8, Ordering::Release);
        OutResponse::Accepted
    }

    fn get_idle_ms(&mut self, id: Option<ReportId>) -> Option<u32> {
        if id.is_none() || id == Some(ReportId::In(0)) {
            Some(KEYBOARD_IDLE_MS.load(Ordering::Acquire))
        } else {
            None
        }
    }

    fn set_idle_ms(&mut self, id: Option<ReportId>, duration_ms: u32) {
        if id.is_none() || id == Some(ReportId::In(0)) {
            KEYBOARD_IDLE_MS.store(duration_ms, Ordering::Release);
        } else {
            DIAGNOSTICS
                .invalid_abi_calls
                .fetch_add(1, Ordering::Relaxed);
        }
    }
}

#[embassy_executor::task]
async fn matrix_task(mut matrix: Imk64Matrix) {
    loop {
        let mut snapshot = [0u16; MATRIX_ROW_COUNT];
        for (row_index, row) in matrix.rows.iter_mut().enumerate() {
            let _ = row.set_high();
            Timer::after_micros(1).await;
            for (col_index, col) in matrix.cols.iter_mut().enumerate() {
                if col.is_high().unwrap_or(false) {
                    snapshot[row_index] |= 1u16 << col_index;
                }
            }
            let _ = row.set_low();
        }

        // Publish a coherent 5-row snapshot. QMK's synchronous facade only
        // loads atomics; all GPIO settling and matrix timing stays async.
        MATRIX_GENERATION.fetch_add(1, Ordering::Relaxed);
        for (destination, value) in MATRIX_ROWS.iter().zip(snapshot) {
            destination.store(value, Ordering::Relaxed);
        }
        MATRIX_GENERATION.fetch_add(1, Ordering::Release);
        MATRIX_READY.store(true, Ordering::Release);
        DIAGNOSTICS.matrix_scans.fetch_add(1, Ordering::Relaxed);
        Timer::after_millis(1).await;
    }
}

#[embassy_executor::task]
async fn qmk_task() {
    unsafe {
        platform_setup();
        protocol_setup();
        ch582_imk64_matrix_backend();
        keyboard_setup();
        protocol_pre_init();
        keyboard_init();
        protocol_post_init();
    }

    loop {
        unsafe {
            protocol_pre_task();
            keyboard_task();
            protocol_post_task();
            housekeeping_task();
        }
        DIAGNOSTICS.qmk_ticks.fetch_add(1, Ordering::Relaxed);
        Timer::after_millis(1).await;
    }
}

async fn keyboard_writer<D: embassy_usb::driver::Driver<'static>>(
    mut writer: HidWriter<'static, D, 8>,
) -> ! {
    loop {
        let report = REPORTS.receive().await;
        loop {
            while !USB_STARTED.load(Ordering::Acquire) || !USB_CONFIGURED.load(Ordering::Acquire) {
                Timer::after_millis(1).await;
            }

            if writer.write(&report).await.is_ok() {
                DIAGNOSTICS.completed.fetch_add(1, Ordering::Relaxed);
                break;
            }
            writer.ready().await;
        }
    }
}

async fn keyboard_reader<D: embassy_usb::driver::Driver<'static>>(
    mut reader: HidReader<'static, D, 1>,
) -> ! {
    let mut output = [0u8; 1];
    loop {
        match reader.read(&mut output).await {
            Ok(1) => {
                KEYBOARD_LEDS.store(output[0], Ordering::Release);
                DIAGNOSTICS.output_reports.fetch_add(1, Ordering::Relaxed);
            }
            Ok(_) => {
                DIAGNOSTICS
                    .invalid_abi_calls
                    .fetch_add(1, Ordering::Relaxed);
            }
            Err(_) => reader.ready().await,
        }
    }
}

async fn usb_task(driver: embassy_ch58x::usb::Driver<'static>) -> ! {
    let mut config = Config::new(0xac26, 0x6402);
    config.manufacturer = Some("IMK");
    config.product = Some("imk64 CH582 Rust QMK");
    config.serial_number = Some("CH582-IMK64-QMK-0001");
    config.max_packet_size_0 = 64;
    config.max_power = 100;

    static CONFIG_DESCRIPTOR: StaticCell<[u8; 128]> = StaticCell::new();
    static BOS_DESCRIPTOR: StaticCell<[u8; 16]> = StaticCell::new();
    static MSOS_DESCRIPTOR: StaticCell<[u8; 16]> = StaticCell::new();
    static CONTROL_BUFFER: StaticCell<[u8; 128]> = StaticCell::new();
    static HID_STATE: StaticCell<HidState> = StaticCell::new();
    static HANDLER: StaticCell<UsbStateHandler> = StaticCell::new();
    static REQUEST_HANDLER: StaticCell<KeyboardRequestHandler> = StaticCell::new();

    let mut builder = Builder::new(
        driver,
        config,
        CONFIG_DESCRIPTOR.init([0; 128]),
        BOS_DESCRIPTOR.init([0; 16]),
        MSOS_DESCRIPTOR.init([0; 16]),
        CONTROL_BUFFER.init([0; 128]),
    );
    builder.handler(HANDLER.init(UsbStateHandler));
    let keyboard = HidReaderWriter::<_, 1, 8>::new(
        &mut builder,
        HID_STATE.init(HidState::new()),
        embassy_usb::class::hid::Config {
            report_descriptor: KEYBOARD_REPORT_DESCRIPTOR,
            request_handler: Some(REQUEST_HANDLER.init(KeyboardRequestHandler)),
            poll_ms: 1,
            max_packet_size: 8,
            hid_subclass: HidSubclass::Boot,
            hid_boot_protocol: HidBootProtocol::Keyboard,
        },
    );
    let (reader, writer) = keyboard.split();
    let mut usb = builder.build();
    join3(usb.run(), keyboard_reader(reader), keyboard_writer(writer)).await;
    unreachable!()
}

#[qingke_rs::entry]
fn main() -> ! {
    let peripherals = embassy_ch58x::init(Default::default());
    let pins = Pins::new(peripherals.GPIOA, peripherals.GPIOB);
    let matrix = Imk64Matrix {
        rows: [
            Output::new(pins.pa1, Level::Low, Drive::MilliAmps5).degrade(),
            Output::new(pins.pa2, Level::Low, Drive::MilliAmps5).degrade(),
            Output::new(pins.pa3, Level::Low, Drive::MilliAmps5).degrade(),
            Output::new(pins.pa4, Level::Low, Drive::MilliAmps5).degrade(),
            Output::new(pins.pa5, Level::Low, Drive::MilliAmps5).degrade(),
        ],
        cols: [
            Input::new(pins.pb4, Pull::Down).degrade(),
            Input::new(pins.pb5, Pull::Down).degrade(),
            Input::new(pins.pb6, Pull::Down).degrade(),
            Input::new(pins.pb7, Pull::Down).degrade(),
            Input::new(pins.pb14, Pull::Down).degrade(),
            Input::new(pins.pb15, Pull::Down).degrade(),
            Input::new(pins.pb16, Pull::Down).degrade(),
            Input::new(pins.pb17, Pull::Down).degrade(),
            Input::new(pins.pb8, Pull::Down).degrade(),
            Input::new(pins.pb9, Pull::Down).degrade(),
            Input::new(pins.pa8, Pull::Down).degrade(),
            Input::new(pins.pb18, Pull::Down).degrade(),
            Input::new(pins.pb19, Pull::Down).degrade(),
            Input::new(pins.pb20, Pull::Down).degrade(),
        ],
    };

    let driver = embassy_ch58x::usb::Driver::new(peripherals.USB, Irqs);
    static EXECUTOR: StaticCell<embassy_ch58x::executor::Executor> = StaticCell::new();
    EXECUTOR
        .init(embassy_ch58x::executor::Executor::new())
        .run(|spawner| {
            spawner.must_spawn(matrix_task(matrix));
            spawner.must_spawn(qmk_task());
            spawner.must_spawn(usb_task_entry(driver));
        })
}

#[embassy_executor::task]
async fn usb_task_entry(driver: embassy_ch58x::usb::Driver<'static>) {
    usb_task(driver).await
}

#[unsafe(no_mangle)]
extern "C" fn ch582_rust_usb_start() {
    USB_STARTED.store(true, Ordering::Release);
}

#[unsafe(no_mangle)]
extern "C" fn ch582_rust_usb_stop() {
    USB_STARTED.store(false, Ordering::Release);
}

#[unsafe(no_mangle)]
unsafe extern "C" fn ch582_rust_usb_try_write(endpoint: u8, data: *const u8, size: usize) -> bool {
    if endpoint != KEYBOARD_ENDPOINT || size != 8 || data.is_null() {
        DIAGNOSTICS
            .invalid_abi_calls
            .fetch_add(1, Ordering::Relaxed);
        return false;
    }
    let mut report = [0u8; 8];
    unsafe { ptr::copy_nonoverlapping(data, report.as_mut_ptr(), report.len()) };
    match REPORTS.try_send(report) {
        Ok(()) => {
            DIAGNOSTICS.queued.fetch_add(1, Ordering::Relaxed);
            true
        }
        Err(TrySendError::Full(report)) => {
            // QMK's host callback cannot apply backpressure. Prefer the newest
            // keyboard state so a release cannot remain stuck behind a stale
            // full queue. No await or blocking critical section is involved.
            DIAGNOSTICS.queue_full.fetch_add(1, Ordering::Relaxed);
            let _ = REPORTS.try_receive();
            if REPORTS.try_send(report).is_ok() {
                DIAGNOSTICS.queued.fetch_add(1, Ordering::Relaxed);
                true
            } else {
                false
            }
        }
    }
}

#[unsafe(no_mangle)]
extern "C" fn ch582_rust_usb_keyboard_leds() -> u8 {
    KEYBOARD_LEDS.load(Ordering::Acquire)
}

#[unsafe(no_mangle)]
unsafe extern "C" fn ch582_rust_matrix_snapshot(rows: *mut u16, row_count: usize) -> bool {
    if rows.is_null() || row_count != MATRIX_ROW_COUNT || !MATRIX_READY.load(Ordering::Acquire) {
        return false;
    }

    for _ in 0..2 {
        let before = MATRIX_GENERATION.load(Ordering::Acquire);
        if before & 1 != 0 {
            continue;
        }

        let mut snapshot = [0u16; MATRIX_ROW_COUNT];
        for (destination, source) in snapshot.iter_mut().zip(MATRIX_ROWS.iter()) {
            *destination = source.load(Ordering::Relaxed);
        }
        let after = MATRIX_GENERATION.load(Ordering::Acquire);
        if before == after {
            unsafe { ptr::copy_nonoverlapping(snapshot.as_ptr(), rows, MATRIX_ROW_COUNT) };
            return true;
        }
    }
    false
}

#[unsafe(no_mangle)]
extern "C" fn ch582_rust_millis() -> u32 {
    Instant::now().as_millis() as u32
}

#[unsafe(no_mangle)]
extern "C" fn timer_init() {}

#[unsafe(no_mangle)]
extern "C" fn timer_read() -> u16 {
    ch582_rust_millis() as u16
}

#[unsafe(no_mangle)]
extern "C" fn timer_read32() -> u32 {
    ch582_rust_millis()
}

#[unsafe(no_mangle)]
extern "C" fn timer_clear() {}

#[unsafe(no_mangle)]
extern "C" fn timer_save() {}

#[unsafe(no_mangle)]
extern "C" fn timer_restore() {}

#[unsafe(no_mangle)]
extern "C" fn ch582_rust_wait_us(micros: u32) {
    // QMK's wait API is synchronous and cannot yield an Embassy task. Matrix
    // settling is implemented by the async scanner, and this target does not
    // enable delay-dependent send-string or lighting drivers, so record and
    // bypass compatibility waits instead of starving USB and matrix futures.
    if micros != 0 {
        DIAGNOSTICS
            .synchronous_wait_requests
            .fetch_add(1, Ordering::Relaxed);
    }
}

#[unsafe(no_mangle)]
unsafe extern "C" fn ch582_rust_critical_enter() -> usize {
    let previous: usize;
    unsafe {
        core::arch::asm!(
            "csrrc {previous}, 0x800, {mask}",
            "nop",
            "nop",
            previous = out(reg) previous,
            mask = in(reg) 0x88usize,
            options(nostack),
        );
    }
    previous & 0x08
}

#[unsafe(no_mangle)]
unsafe extern "C" fn ch582_rust_critical_exit(was_enabled: usize) {
    if was_enabled != 0 {
        unsafe {
            core::arch::asm!(
                "csrrs x0, 0x800, {mask}",
                "nop",
                "nop",
                mask = in(reg) 0x08usize,
                options(nostack),
            );
        }
    }
}

#[unsafe(no_mangle)]
extern "C" fn ch582_rust_reset(_bootloader: bool) -> ! {
    const PFIC_SCTLR: *mut u32 = 0xe000_ed10 as *mut u32;
    unsafe {
        let control = ptr::read_volatile(PFIC_SCTLR);
        ptr::write_volatile(PFIC_SCTLR, control | (1 << 31));
    }
    loop {
        core::hint::spin_loop();
    }
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo<'_>) -> ! {
    loop {
        core::hint::spin_loop();
    }
}
