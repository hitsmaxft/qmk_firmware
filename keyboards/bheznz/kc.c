#include "quantum.h"
#include "bheznz.h"


/** a simple keycode to ascii converter **/

char* keycode_to_ascii(uint16_t keycode) {

    //reuse char array
    static char ascii[3] = {' ', ' ', '\0'}; // 初始化为全空字符串
        ascii[0] = ' ';
    if (keycode >= KC_A && keycode <= KC_Z) {
        ascii[1] = 'A' + ((keycode - KC_A) % 26);
    } else if (keycode >= KC_1 && keycode <= KC_9) {
        ascii[1] = '1' + ((keycode - KC_1) % 9);
    } else {
        switch (keycode) {
            case KC_0:
                ascii[1] = '0';
                break;
            case KC_ENTER:
            case KC_ESCAPE:
            case KC_BACKSPACE:
            case KC_TAB:
            case KC_SPACE:
            case KC_MINUS:
            case KC_EQUAL:
            case KC_LEFT_BRACKET:
            case KC_RIGHT_BRACKET:
            case KC_BACKSLASH:
            case KC_NONUS_HASH:
            case KC_SEMICOLON:
            case KC_QUOTE:
            case KC_GRAVE:
            case KC_COMMA:
            case KC_DOT:
            case KC_SLASH:
            case KC_CAPS_LOCK:
                ascii[1] = (char[]){'n', 0x1B, 0x08, 't', ' ', '-', '=', '[', ']', '\\', '#', ';', '\'', '`', ',', '.', '/'}[keycode - KC_ENTER];
                break;
            case RGB_TOG:
                return "RGBT";
                break;
            case RGB_MOD:
                return "RM+";
                break;
            case RGB_RMOD:
                return "RM-";
                break;
            case RGB_VAI :
                return "RV+";
                break;
            case RGB_VAD :
                return "RV-";
                break;
            default:
                ascii[0] = '-'; // 对于不表示打印字符的按键，返回空字符串
                ascii[1] = '-'; // 对于不表示打印字符的按键，返回空字符串
                break;
        }
    }

    return ascii;
}


// 定义一个足够大的数组来存储按键代码
// QMK 中，一个 keycode 通常定义为 uint16_t 类型
#define KEYCODE_HISTORY_SIZE 3
static uint16_t keycode_history[KEYCODE_HISTORY_SIZE] = {KC_NO, KC_NO, KC_NO};

// 更新存储的按键代码历史
void add_keycode_to_history(uint16_t keycode) {
    // 将历史记录中的每个键码往后移动一个位置
    for (int i = KEYCODE_HISTORY_SIZE - 1; i > 0; i--) {
        keycode_history[i] = keycode_history[i - 1];
    }
    // 在数组的起始位置插入新的键码
    keycode_history[0] = keycode;
}


// 打印最近的三个按键代码
// TODO limit buffer size
void sprint_recent_keycodes(char * buffer, uint8_t size) {
    char *buff_ptr = buffer;
    *buff_ptr = '\0';

    for (int i = 0; i < KEYCODE_HISTORY_SIZE; i++) {
        // 获取键码对应的字符
        char *key_char = keycode_to_ascii(keycode_history[i]);
        // move pointer
        buff_ptr += snprintf(buff_ptr, 4, "%s ", key_char);
    }
    if( buff_ptr != buffer ) {
        //remove last space
        *(buff_ptr-1) = '\0';
    }
}

// 打印最近的三个按键代码
void print_recent_keycodes(void) {
    // 输出存储的按键代码到控制台
    dprint("Recent keycodes: ");
    for (int i = 0; i < KEYCODE_HISTORY_SIZE; i++) {
        uprintf("%s ", keycode_to_ascii(keycode_history[i]));
    }
    dprint("\n");
}

