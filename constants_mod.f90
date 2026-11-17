module constants_mod
!==============================================================================#
! CONSTANTS_MOD
!------------------------------------------------------------------------------#
! Author:  Ed Higgins <ed.j.higgins@gmail.com>
!------------------------------------------------------------------------------#
! Version: 0.1.1, 2026-11-12
!------------------------------------------------------------------------------#
! This code is distributed under the GLWTPL license.
!==============================================================================#
  use, intrinsic :: iso_c_binding, only : cf => c_float, ci => c_int
  implicit none

  public

  real, parameter :: PI = 4*atan(1.0)

  ! GLFW key codes
integer(ci), parameter :: KEY_SPACE = 32
integer(ci), parameter :: KEY_APOSTROPHE = 39 ! '
integer(ci), parameter :: KEY_COMMA = 44 ! ,
integer(ci), parameter :: KEY_MINUS = 45 ! -
integer(ci), parameter :: KEY_PERIOD = 46 ! .
integer(ci), parameter :: KEY_SLASH = 47 ! /
integer(ci), parameter :: KEY_0 = 48
integer(ci), parameter :: KEY_1 = 49
integer(ci), parameter :: KEY_2 = 50
integer(ci), parameter :: KEY_3 = 51
integer(ci), parameter :: KEY_4 = 52
integer(ci), parameter :: KEY_5 = 53
integer(ci), parameter :: KEY_6 = 54
integer(ci), parameter :: KEY_7 = 55
integer(ci), parameter :: KEY_8 = 56
integer(ci), parameter :: KEY_9 = 57
integer(ci), parameter :: KEY_SEMICOLON = 59 ! ;
integer(ci), parameter :: KEY_EQUAL = 61 ! =
integer(ci), parameter :: KEY_A = 65
integer(ci), parameter :: KEY_B = 66
integer(ci), parameter :: KEY_C = 67
integer(ci), parameter :: KEY_D = 68
integer(ci), parameter :: KEY_E = 69
integer(ci), parameter :: KEY_F = 70
integer(ci), parameter :: KEY_G = 71
integer(ci), parameter :: KEY_H = 72
integer(ci), parameter :: KEY_I = 73
integer(ci), parameter :: KEY_J = 74
integer(ci), parameter :: KEY_K = 75
integer(ci), parameter :: KEY_L = 76
integer(ci), parameter :: KEY_M = 77
integer(ci), parameter :: KEY_N = 78
integer(ci), parameter :: KEY_O = 79
integer(ci), parameter :: KEY_P = 80
integer(ci), parameter :: KEY_Q = 81
integer(ci), parameter :: KEY_R = 82
integer(ci), parameter :: KEY_S = 83
integer(ci), parameter :: KEY_T = 84
integer(ci), parameter :: KEY_U = 85
integer(ci), parameter :: KEY_V = 86
integer(ci), parameter :: KEY_W = 87
integer(ci), parameter :: KEY_X = 88
integer(ci), parameter :: KEY_Y = 89
integer(ci), parameter :: KEY_Z = 90
integer(ci), parameter :: KEY_LEFT_BRACKET = 91 ! [
integer(ci), parameter :: KEY_BACKSLASH = 92 ! \
integer(ci), parameter :: KEY_RIGHT_BRACKET = 93 ! ]
integer(ci), parameter :: KEY_GRAVE_ACCENT = 96 ! `
integer(ci), parameter :: KEY_WORLD_1 = 161 ! non-US #1
integer(ci), parameter :: KEY_WORLD_2 = 162 ! non-US #2
integer(ci), parameter :: KEY_ESCAPE = 256
integer(ci), parameter :: KEY_ENTER = 257
integer(ci), parameter :: KEY_TAB = 258
integer(ci), parameter :: KEY_BACKSPACE = 259
integer(ci), parameter :: KEY_INSERT = 260
integer(ci), parameter :: KEY_DELETE = 261
integer(ci), parameter :: KEY_RIGHT = 262
integer(ci), parameter :: KEY_LEFT = 263
integer(ci), parameter :: KEY_DOWN = 264
integer(ci), parameter :: KEY_UP = 265
integer(ci), parameter :: KEY_PAGE_UP = 266
integer(ci), parameter :: KEY_PAGE_DOWN = 267
integer(ci), parameter :: KEY_HOME = 268
integer(ci), parameter :: KEY_END = 269
integer(ci), parameter :: KEY_CAPS_LOCK = 280
integer(ci), parameter :: KEY_SCROLL_LOCK = 281
integer(ci), parameter :: KEY_NUM_LOCK = 282
integer(ci), parameter :: KEY_PRINT_SCREEN = 283
integer(ci), parameter :: KEY_PAUSE = 284
integer(ci), parameter :: KEY_F1 = 290
integer(ci), parameter :: KEY_F2 = 291
integer(ci), parameter :: KEY_F3 = 292
integer(ci), parameter :: KEY_F4 = 293
integer(ci), parameter :: KEY_F5 = 294
integer(ci), parameter :: KEY_F6 = 295
integer(ci), parameter :: KEY_F7 = 296
integer(ci), parameter :: KEY_F8 = 297
integer(ci), parameter :: KEY_F9 = 298
integer(ci), parameter :: KEY_F10 = 299
integer(ci), parameter :: KEY_F11 = 300
integer(ci), parameter :: KEY_F12 = 301
integer(ci), parameter :: KEY_F13 = 302
integer(ci), parameter :: KEY_F14 = 303
integer(ci), parameter :: KEY_F15 = 304
integer(ci), parameter :: KEY_F16 = 305
integer(ci), parameter :: KEY_F17 = 306
integer(ci), parameter :: KEY_F18 = 307
integer(ci), parameter :: KEY_F19 = 308
integer(ci), parameter :: KEY_F20 = 309
integer(ci), parameter :: KEY_F21 = 310
integer(ci), parameter :: KEY_F22 = 311
integer(ci), parameter :: KEY_F23 = 312
integer(ci), parameter :: KEY_F24 = 313
integer(ci), parameter :: KEY_F25 = 314
integer(ci), parameter :: KEY_KP_0 = 320
integer(ci), parameter :: KEY_KP_1 = 321
integer(ci), parameter :: KEY_KP_2 = 322
integer(ci), parameter :: KEY_KP_3 = 323
integer(ci), parameter :: KEY_KP_4 = 324
integer(ci), parameter :: KEY_KP_5 = 325
integer(ci), parameter :: KEY_KP_6 = 326
integer(ci), parameter :: KEY_KP_7 = 327
integer(ci), parameter :: KEY_KP_8 = 328
integer(ci), parameter :: KEY_KP_9 = 329
integer(ci), parameter :: KEY_KP_DECIMAL = 330
integer(ci), parameter :: KEY_KP_DIVIDE = 331
integer(ci), parameter :: KEY_KP_MULTIPLY = 332
integer(ci), parameter :: KEY_KP_SUBTRACT = 333
integer(ci), parameter :: KEY_KP_ADD = 334
integer(ci), parameter :: KEY_KP_ENTER = 335
integer(ci), parameter :: KEY_KP_EQUAL = 336
integer(ci), parameter :: KEY_LEFT_SHIFT = 340
integer(ci), parameter :: KEY_LEFT_CONTROL = 341
integer(ci), parameter :: KEY_LEFT_ALT = 342
integer(ci), parameter :: KEY_LEFT_SUPER = 343
integer(ci), parameter :: KEY_RIGHT_SHIFT = 344
integer(ci), parameter :: KEY_RIGHT_CONTROL = 345
integer(ci), parameter :: KEY_RIGHT_ALT = 346
integer(ci), parameter :: KEY_RIGHT_SUPER = 347
integer(ci), parameter :: KEY_MENU = 348
integer(ci), parameter :: KEY_LAST = 348
end module constants_mod
