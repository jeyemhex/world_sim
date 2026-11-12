module camera_mod
!==============================================================================#
! CAMERA_MOD
!------------------------------------------------------------------------------#
! Author:  Ed Higgins <ed.j.higgins@gmail.com>
!------------------------------------------------------------------------------#
! Version: 0.1.1, 2026-11-12
!------------------------------------------------------------------------------#
! This code is distributed under the GLWTPL license.
!==============================================================================#
  use, intrinsic :: iso_c_binding, only : cf => c_float, ci => c_int
  implicit none

  private

  ! GLFW key codes
  integer(ci), parameter :: KEY_SPACE = 32
  integer(ci), parameter :: KEY_A     = 65
  integer(ci), parameter :: KEY_D     = 68
  integer(ci), parameter :: KEY_S     = 83
  integer(ci), parameter :: KEY_W     = 87
  integer(ci), parameter :: KEY_RIGHT = 262
  integer(ci), parameter :: KEY_LEFT  = 263
  integer(ci), parameter :: KEY_DOWN  = 264
  integer(ci), parameter :: KEY_UP    = 265
  integer(ci), parameter :: KEY_SHIFT = 340

  type, public :: camera_t
    real(cf) :: pos(3) = [0.0_cf, 0.0_cf, 25.0_cf]
    real(cf) :: yaw    = 0.785_cf
    real(cf) :: pitch  = -0.45_cf
    real(cf) :: speed  = 10.0_cf
    real(cf) :: turn_speed = 1.0_cf
  end type camera_t

  interface
    function vk_is_key_down(key) bind(C, name="vk_is_key_down")
      import :: ci
      integer(ci) :: vk_is_key_down
      integer(ci), value :: key
    end function vk_is_key_down
  end interface

  public :: camera_update, camera_get_mvp

contains
  subroutine camera_update(cam, dt)
    type(camera_t), intent(inout) :: cam
    real(cf),  intent(in)    :: dt
    real(cf) :: fwd(2), right(2)

    ! Look rotation (arrow keys)
    if (vk_is_key_down(KEY_LEFT)  /= 0) cam%yaw = cam%yaw + cam%turn_speed*dt
    if (vk_is_key_down(KEY_RIGHT) /= 0) cam%yaw = cam%yaw - cam%turn_speed*dt
    if (vk_is_key_down(KEY_UP)    /= 0) cam%pitch = cam%pitch + cam%turn_speed*dt
    if (vk_is_key_down(KEY_DOWN)  /= 0) cam%pitch = cam%pitch - cam%turn_speed*dt

    fwd = [cos(cam%yaw), sin(cam%yaw)] * cam%speed
    right = [sin(cam%yaw), -cos(cam%yaw)] * cam%speed

    ! Translation (WASD + space + l-shift)
    if (vk_is_key_down(KEY_W)  /= 0) cam%pos(1:2) = cam%pos(1:2) + fwd*dt
    if (vk_is_key_down(KEY_S)  /= 0) cam%pos(1:2) = cam%pos(1:2) - fwd*dt
    if (vk_is_key_down(KEY_D)  /= 0) cam%pos(1:2) = cam%pos(1:2) + right*dt
    if (vk_is_key_down(KEY_A)  /= 0) cam%pos(1:2) = cam%pos(1:2) - right*dt
    if (vk_is_key_down(KEY_SPACE) /= 0) cam%pos(3) = cam%pos(3) + cam%speed*dt
    if (vk_is_key_down(KEY_SHIFT) /= 0) cam%pos(3) = cam%pos(3) - cam%speed*dt
  end subroutine camera_update

  subroutine camera_get_mvp(cam, aspect, mvp)
    type(camera_t), intent(in)  :: cam
    real(cf),  intent(in)  :: aspect
    real(cf),  intent(out) :: mvp(4,4)

    real(cf) :: view(4,4), proj(4,4)
    real(cf) :: f(3), s(3), u(3), world_up(3)
    real(cf) :: fov_rad, tan_half_fov, z_near, z_far

    ! Forward vector from spehrical pitch/yaw (Z is up)
    f = [ cos(cam%pitch)*cos(cam%yaw), &
          cos(cam%pitch)*sin(cam%yaw), &
          sin(cam%pitch)]

    world_up = [0.0_cf, 0.0_cf, 1.0_cf]
    s = cross(f, world_up)
    s = s / norm2(s)
    u = cross(s, f)

    ! Look at view matrix
    view = 0.0_cf
    view(1, 1:3) = s
    view(2, 1:3) = u
    view(3, 1:3) = -f
    view(1, 4)   = -dot_product(s, cam%pos)
    view(2, 4)   = -dot_product(u, cam%pos)
    view(3, 4)   =  dot_product(f, cam%pos)
    view(4, 4)   =  1.0_cf

    ! Vulkan perspective matrix (0-1 depth, y-flipped)
    fov_rad = 1.0472_cf
    tan_half_fov = tan(fov_rad * 0.5_cf)
    z_near = 0.5_cf
    z_far  = 500.0_cf

    proj = 0.0_cf
    proj(1,1) = 1.0_cf / (aspect * tan_half_fov)
    proj(2,2) = -1.0_cf / tan_half_fov
    proj(3,3) = z_far / (z_near - z_far)
    proj(4,3) = -1.0_cf
    proj(3,4) = (z_near*z_far) / (z_near - z_far)

    mvp = matmul(proj, view)

  end subroutine camera_get_mvp

  pure function cross(a,b) result(c)
    real(cf) :: c(3)
    real(cf), intent(in) :: a(3), b(3)

    c(1) = a(2)*b(3) - a(3)*b(2)
    c(2) = a(3)*b(1) - a(1)*b(3)
    c(3) = a(1)*b(2) - a(2)*b(1)

  end function cross

end module camera_mod
