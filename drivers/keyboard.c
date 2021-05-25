#include <interrupt.h>
#include <cia.h>
#include <devfile.h>
#include <event.h>
#include <input.h>
#include <ioreq.h>
#include <driver.h>
#include <keysym.h>
#include <sys/errno.h>

#define DEBUG 0
#include <debug.h>

typedef enum KeyCode {
  KEY_GRAVE = 0x00,
  KEY_1 = 0x01,
  KEY_2 = 0x02,
  KEY_3 = 0x03,
  KEY_4 = 0x04,
  KEY_5 = 0x05,
  KEY_6 = 0x06,
  KEY_7 = 0x07,
  KEY_8 = 0x08,
  KEY_9 = 0x09,
  KEY_0 = 0x0a,
  KEY_MINUS = 0x0b,
  KEY_EQUAL = 0x0c,
  KEY_BACKSLASH = 0x0d,
  KEY_KP_0 = 0x0f,
  KEY_Q = 0x10,
  KEY_W = 0x11,
  KEY_E = 0x12,
  KEY_R = 0x13,
  KEY_T = 0x14,
  KEY_Y = 0x15,
  KEY_U = 0x16,
  KEY_I = 0x17,
  KEY_O = 0x18,
  KEY_P = 0x19,
  KEY_LBRACKET = 0x1a,
  KEY_RBRACKET = 0x1b,
  KEY_KP_1 = 0x1d,
  KEY_KP_2 = 0x1e,
  KEY_KP_3 = 0x1f,
  KEY_A = 0x20,
  KEY_S = 0x21,
  KEY_D = 0x22,
  KEY_F = 0x23,
  KEY_G = 0x24,
  KEY_H = 0x25,
  KEY_J = 0x26,
  KEY_K = 0x27,
  KEY_L = 0x28,
  KEY_SEMICOLON = 0x29,
  KEY_QUOTE = 0x2a,
  KEY_DEAD1 = 0x2b,
  KEY_KP_4 = 0x2d,
  KEY_KP_5 = 0x2e,
  KEY_KP_6 = 0x2f,
  KEY_DEAD2 = 0x30,
  KEY_Z = 0x31,
  KEY_X = 0x32,
  KEY_C = 0x33,
  KEY_V = 0x34,
  KEY_B = 0x35,
  KEY_N = 0x36,
  KEY_M = 0x37,
  KEY_COMMA = 0x38,
  KEY_PERIOD = 0x39,
  KEY_SLASH = 0x3a,
  KEY_KP_DECIMAL = 0x3c,
  KEY_KP_7 = 0x3d,
  KEY_KP_8 = 0x3e,
  KEY_KP_9 = 0x3f,
  KEY_SPACE = 0x40,
  KEY_BACKSPACE = 0x41,
  KEY_TAB = 0x42,
  KEY_KP_ENTER = 0x43,
  KEY_RETURN = 0x44,
  KEY_ESCAPE = 0x45,
  KEY_DELETE = 0x46,
  KEY_KP_SUBSTRACT = 0x4a,
  KEY_UP = 0x4c,
  KEY_DOWN = 0x4d,
  KEY_RIGHT = 0x4e,
  KEY_LEFT = 0x4f,
  KEY_F1 = 0x50,
  KEY_F2 = 0x51,
  KEY_F3 = 0x52,
  KEY_F4 = 0x53,
  KEY_F5 = 0x54,
  KEY_F6 = 0x55,
  KEY_F7 = 0x56,
  KEY_F8 = 0x57,
  KEY_F9 = 0x58,
  KEY_F10 = 0x59,
  KEY_KP_LPAREN = 0x5a,
  KEY_KP_RPAREN = 0x5b,
  KEY_KP_DIVIDE = 0x5c,
  KEY_KP_MULTIPLY = 0x5d,
  KEY_KP_ADD = 0x5e,
  KEY_HELP = 0x5f,
  KEY_LSHIFT = 0x60,
  KEY_RSHIFT = 0x61,
  KEY_CAPSLOCK = 0x62,
  KEY_CONTROL = 0x63,
  KEY_LALT = 0x64,
  KEY_RALT = 0x65,
  KEY_LAMIGA = 0x66,
  KEY_RAMIGA = 0x67,
} __packed KeyCode_t;

static const KeySym_t KeyMap[][2] = {
  [KEY_GRAVE] = {KS_grave, KS_asciitilde},
  [KEY_1] = {KS_1, KS_exclam},
  [KEY_2] = {KS_2, KS_at},
  [KEY_3] = {KS_3, KS_numbersign},
  [KEY_4] = {KS_4, KS_dollar},
  [KEY_5] = {KS_5, KS_percent},
  [KEY_6] = {KS_6, KS_asciicircum},
  [KEY_7] = {KS_7, KS_ampersand},
  [KEY_8] = {KS_8, KS_asterisk},
  [KEY_9] = {KS_9, KS_parenleft},
  [KEY_0] = {KS_0, KS_parenright},
  [KEY_MINUS] = {KS_minus, KS_underscore},
  [KEY_EQUAL] = {KS_equal, KS_plus},
  [KEY_BACKSLASH] = {KS_backslash, KS_bar},
  [KEY_KP_0] = {KS_KP_0, KS_KP_Insert},
  [KEY_Q] = {KS_q, KS_Q},
  [KEY_W] = {KS_w, KS_W},
  [KEY_E] = {KS_e, KS_E},
  [KEY_R] = {KS_r, KS_R},
  [KEY_T] = {KS_t, KS_T},
  [KEY_Y] = {KS_y, KS_Y},
  [KEY_U] = {KS_u, KS_U},
  [KEY_I] = {KS_i, KS_I},
  [KEY_O] = {KS_o, KS_O},
  [KEY_P] = {KS_p, KS_P},
  [KEY_LBRACKET] = {KS_bracketleft, KS_braceleft},
  [KEY_RBRACKET] = {KS_bracketright, KS_braceright},
  [KEY_KP_1] = {KS_KP_1, KS_KP_End},
  [KEY_KP_2] = {KS_KP_2, KS_KP_Down},
  [KEY_KP_3] = {KS_KP_3, KS_KP_Next},
  [KEY_A] = {KS_a, KS_A},
  [KEY_S] = {KS_s, KS_S},
  [KEY_D] = {KS_d, KS_D},
  [KEY_F] = {KS_f, KS_F},
  [KEY_G] = {KS_g, KS_G},
  [KEY_H] = {KS_h, KS_H},
  [KEY_J] = {KS_j, KS_J},
  [KEY_K] = {KS_k, KS_K},
  [KEY_L] = {KS_l, KS_L},
  [KEY_SEMICOLON] = {KS_semicolon, KS_colon},
  [KEY_QUOTE] = {KS_apostrophe, KS_quotedbl},
  [KEY_KP_4] = {KS_KP_4, KS_KP_Left},
  [KEY_KP_5] = {KS_KP_5},
  [KEY_KP_6] = {KS_KP_6, KS_KP_Right},
  [KEY_Z] = {KS_z, KS_Z},
  [KEY_X] = {KS_x, KS_X},
  [KEY_C] = {KS_c, KS_C},
  [KEY_V] = {KS_v, KS_V},
  [KEY_B] = {KS_b, KS_B},
  [KEY_N] = {KS_n, KS_N},
  [KEY_M] = {KS_m, KS_M},
  [KEY_COMMA] = {KS_comma, KS_less},
  [KEY_PERIOD] = {KS_period, KS_greater},
  [KEY_SLASH] = {KS_slash, KS_question},
  [KEY_KP_DECIMAL] = {KS_KP_Decimal, KS_KP_Delete},
  [KEY_KP_7] = {KS_KP_7, KS_KP_Home},
  [KEY_KP_8] = {KS_KP_8, KS_KP_Up},
  [KEY_KP_9] = {KS_KP_9, KS_KP_Prior},
  [KEY_SPACE] = {KS_space, KS_space},
  [KEY_BACKSPACE] = {KS_Backspace},
  [KEY_TAB] = {KS_Tab},
  [KEY_KP_ENTER] = {KS_KP_Enter},
  [KEY_RETURN] = {KS_Return},
  [KEY_ESCAPE] = {KS_Escape},
  [KEY_DELETE] = {KS_Delete},
  [KEY_KP_SUBSTRACT] = {KS_KP_Subtract},
  [KEY_UP] = {KS_Up},
  [KEY_DOWN] = {KS_Down},
  [KEY_RIGHT] = {KS_Right},
  [KEY_LEFT] = {KS_Left},
  [KEY_F1] = {KS_f1},
  [KEY_F2] = {KS_f2},
  [KEY_F3] = {KS_f3},
  [KEY_F4] = {KS_f4},
  [KEY_F5] = {KS_f5},
  [KEY_F6] = {KS_f6},
  [KEY_F7] = {KS_f7},
  [KEY_F8] = {KS_f8},
  [KEY_F9] = {KS_f9},
  [KEY_F10] = {KS_f10},
  [KEY_KP_LPAREN] = {KS_KP_Paren_Left},
  [KEY_KP_RPAREN] = {KS_KP_Paren_Right},
  [KEY_KP_DIVIDE] = {KS_KP_Divide},
  [KEY_KP_MULTIPLY] = {KS_KP_Multiply},
  [KEY_KP_ADD] = {KS_KP_Add},
  [KEY_HELP] = {KS_Help},
  [KEY_LSHIFT] = {KS_Shift_L},
  [KEY_RSHIFT] = {KS_Shift_R},
  [KEY_CAPSLOCK] = {KS_Caps_Lock},
  [KEY_CONTROL] = {KS_Control_L},
  [KEY_LALT] = {KS_Alt_L},
  [KEY_RALT] = {KS_Alt_R},
  [KEY_LAMIGA] = {KS_Meta_L},
  [KEY_RAMIGA] = {KS_Meta_R},

  /* Error codes. */
  [0x78] = {KS_unknown}, /* Reset warning. */
  [0xf9] = {KS_unknown}, /* Last key code bad. */
  [0xfa] = {KS_unknown}, /* Keyboard key buffer overflow. */
  [0xfc] = {KS_unknown}, /* Keyboard self-test fail. */
  [0xfd] = {KS_unknown}, /* Initiate power-up key stream. */
  [0xfe] = {KS_unknown}, /* Terminate power-up key stream. */
  [0xff] = {KS_unknown}, /* ??? */
};

typedef enum KeyMod {
  MOD_LSHIFT = BIT(0),
  MOD_RSHIFT = BIT(1),
  MOD_CAPSLOCK = BIT(2),
  MOD_CONTROL = BIT(3),
  MOD_LALT = BIT(4),
  MOD_RALT = BIT(5),
  MOD_LAMIGA = BIT(6),
  MOD_RAMIGA = BIT(7),

  MOD_SHIFT = MOD_LSHIFT | MOD_RSHIFT,
} __packed KeyMod_t;

typedef struct KeyboardDev {
  QueueHandle_t eventQ;
  IntServer_t intr;
  DevFile_t *file;
  EventWaitList_t readEvent;
  CIATimer_t *timer;
  KeyMod_t modifier;
  KeyCode_t code;
} KeyboardDev_t;

static int KeyboardOpen(DevFile_t *, FileFlags_t);
static int KeyboardClose(DevFile_t *, FileFlags_t);
static int KeyboardRead(DevFile_t *, IoReq_t *);
static int KeyboardEvent(DevFile_t *, EvAction_t, EvFilter_t);

static DevFileOps_t KeyboardOps = {
  .type = DT_OTHER,
  .open = KeyboardOpen,
  .close = KeyboardClose,
  .read = KeyboardRead,
  .event = KeyboardEvent,
};

static int KeyboardOpen(DevFile_t *dev, FileFlags_t flags) {
  if (flags & F_WRITE)
    return EACCES;

  if (!dev->usecnt) {
    KeyboardDev_t *kbd = dev->data;

    kbd->eventQ = InputEventQueueCreate();
    /* Register keyboard interrupt. */
    AddIntServer(PortsChain, &kbd->intr);
    /* Set to input mode. */
    BCLR(ciaa.ciacra, CIACRAB_SPMODE);
    /* Enable keyboard interrupt.
     * The keyboard is attached to CIA-A serial port. */
    WriteICR(CIAA, CIAICRF_SETCLR | CIAICRF_SP);
  }

  return 0;
}

static int KeyboardClose(DevFile_t *dev, FileFlags_t flags __unused) {
  if (!dev->usecnt) {
    KeyboardDev_t *kbd = dev->data;

    RemIntServer(&kbd->intr);
    InputEventQueueDelete(kbd->eventQ);
  }

  return 0;
}

static int KeyboardRead(DevFile_t *dev, IoReq_t *io) {
  KeyboardDev_t *kbd = dev->data;
  return InputEventRead(kbd->eventQ, io);
}

static int KeyboardEvent(DevFile_t *dev, EvAction_t act, EvFilter_t filt) {
  KeyboardDev_t *kbd = dev->data;

  if (filt == EVFILT_READ)
    return EventMonitor(&kbd->readEvent, act);
  return EINVAL;
}

static void ReadKeyEvent(KeyboardDev_t *kbd, uint8_t raw) {
  DLOG("keyboard: reported raw code $%02x\n", raw);

  KeyCode_t code = raw & 0x7f;
  KeyMod_t change = code >= KEY_LSHIFT ? BIT(code - KEY_LSHIFT) : 0;
  InputEvent_t ev;

  /* Process key modifiers */
  if (raw & 0x80) {
    kbd->modifier &= ~change;
    ev.kind = IE_KEYBOARD_UP;
  } else {
    kbd->modifier |= change;
    ev.kind = IE_KEYBOARD_DOWN;
  }

  int shift = ((kbd->modifier & MOD_SHIFT) && (code < KEY_LSHIFT)) ? 1 : 0;
  ev.value = KeyMap[code][shift];

  /* Report if not a dead key. */
  if (ev.value) {
    InputEventInjectFromISR(kbd->eventQ, &ev, 1);
    EventNotifyFromISR(&kbd->readEvent);
    DLOG("keyboard: notify read listeners!\n");
  }
}

static void KeyboardIntHandler(void *data) {
  KeyboardDev_t *kbd = data;
  CIA_t cia = CIAA;
  if (SampleICR(cia, CIAICRF_SP)) {
    /* Read keyboard data register. Yeah, it's negated. */
    uint8_t sdr = ~cia->ciasdr;
    /* Send handshake.
     * 1) Set serial port to output mode.
     * 2) Wait for at least 85us for handshake to be registered.
     * 3) Set back to input mode. */
    BSET(cia->ciacra, CIACRAB_SPMODE);
    WaitTimerSpin(kbd->timer, TIMER_US(85));
    BCLR(cia->ciacra, CIACRAB_SPMODE);
    /* Save raw key in the queue. Filter out exceptional conditions. */
    uint8_t raw = (sdr >> 1) | (sdr << 7);
    if ((raw & 0x7f) <= KEY_RAMIGA)
      ReadKeyEvent(kbd, raw);
  }
}

static int KeyboardAttach(Driver_t *drv) {
  KeyboardDev_t *kbd = drv->state;

  kbd->timer = AcquireTimer(TIMER_ANY);
  if (!kbd->timer)
    return ENXIO;

  TAILQ_INIT(&kbd->readEvent);
  kbd->intr = INTSERVER(-10, (ISR_t)KeyboardIntHandler, (void *)kbd);

  int error;
  if ((error = AddDevFile("keyboard", &KeyboardOps, &kbd->file)))
    return error;

  kbd->file->data = kbd;
  return 0;
}

Driver_t Keyboard = {
  .name = "keyboard",
  .attach = KeyboardAttach,
  .size = sizeof(KeyboardDev_t),
};
