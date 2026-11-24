#include QMK_KEYBOARD_H

// Copy / paste / undo combos (from the Vial layout)
const uint16_t PROGMEM combo_copy[]  = {KC_S, KC_C, COMBO_END};
const uint16_t PROGMEM combo_paste[] = {KC_T, KC_D, COMBO_END};
const uint16_t PROGMEM combo_undo[]  = {KC_A, KC_Z, COMBO_END};

combo_t key_combos[] = {
    COMBO(combo_copy, LCTL(KC_C)),
    COMBO(combo_paste, LCTL(KC_V)),
    COMBO(combo_undo, LCTL(KC_Z)),
};
