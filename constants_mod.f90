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
  integer(ci), parameter :: KEY_A     = 65
  integer(ci), parameter :: KEY_D     = 68
  integer(ci), parameter :: KEY_M     = 77
  integer(ci), parameter :: KEY_Q     = 81
  integer(ci), parameter :: KEY_S     = 83
  integer(ci), parameter :: KEY_W     = 87
  integer(ci), parameter :: KEY_RIGHT = 262
  integer(ci), parameter :: KEY_LEFT  = 263
  integer(ci), parameter :: KEY_DOWN  = 264
  integer(ci), parameter :: KEY_UP    = 265
  integer(ci), parameter :: KEY_SHIFT = 340

end module constants_mod
