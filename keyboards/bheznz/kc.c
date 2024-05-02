#include "quantum.h"
#include "bheznz.h"


char* keycode_to_ascii(uint16_t keycode) {
    static char ascii[2] = {' ', ' '}; // 初始化为全空字符串

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
void sprint_recent_keycodes(char * buffer) {
    // 为字符串结束符预留空间，以及每个键码对应两个字符（包括空格或特殊符号）
    char *buff_ptr = buffer; // 指针，用于追踪当前写入的位置
    for (int i = 0; i < KEYCODE_HISTORY_SIZE; i++) {
        // 获取键码对应的字符
        char *key_char = keycode_to_ascii(keycode_history[i]);
        // 格式化并存储到buffer中，使用sprintf返回写入字符的个数准备下一次写入
        buff_ptr += sprintf(buff_ptr, "%s ", key_char);
    }
    if( buff_ptr != buffer ) { // 如果buffer非空，退回最后的空格写入字符串的结束符
     *(buff_ptr-1) = '0';
    } else {
        *buff_ptr = '0'; // 如果buffer为空（没有键码历史），则写入字符串结束符
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

