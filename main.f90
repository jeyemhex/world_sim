program world_sim
!==============================================================================#
! WORLD_SIM
!------------------------------------------------------------------------------#
! Author:  Ed Higgins <ed.j.higgins@gmail.com>
!------------------------------------------------------------------------------#
! Version: 0.1.1, 2026-11-03
!------------------------------------------------------------------------------#
! This code is distributed under the GLWTPL license.
!==============================================================================#
  use, intrinsic :: iso_c_binding, only : cf => c_float, ci => c_int, cc => c_char, c_ptr, &
                    c_associated, c_null_char, c_f_pointer
  use :: camera_mod
  use :: world_mod
  implicit none

  interface
    function vk_init(width, height, title) bind(C, name="vk_init")
      import :: ci, cc
      integer(ci)                              :: vk_init
      integer(ci),                  value      :: width, height
      character(cc), dimension(*), intent(in) :: title
    end function vk_init

    function vk_allocate_heightmap(grid_w, grid_h) bind(C, name="vk_allocate_heightmap")
      import :: ci, c_ptr
      type(c_ptr) :: vk_allocate_heightmap
      integer(ci), value :: grid_h, grid_w
    end function vk_allocate_heightmap

    function vk_should_close() bind(C, name="vk_should_close")
      import :: ci
      integer(ci) :: vk_should_close
    end function vk_should_close

    subroutine vk_draw_frame(mvp, cam_pos, view_radius, view_mode) bind(C, name="vk_draw_frame")
      import :: cf, ci
      real(cf), dimension(*), intent(in) :: mvp
      real(cf), dimension(*), intent(in) :: cam_pos
      integer(ci), value :: view_radius
      integer(ci), value :: view_mode
    end subroutine vk_draw_frame

    subroutine vk_cleanup() bind(C, name="vk_cleanup")
    end subroutine vk_cleanup
  end interface

  integer(ci), parameter :: GRID_W = 1024, GRID_H = 1024
  integer(ci), parameter :: VIEW_RADIUS = 512_ci
  real(cf),    parameter :: Z_SCALE     = 45.0_cf

  integer(ci) :: status
  type(c_ptr) :: raw_gpu_ptr
  type(cell_t), pointer :: world(:,:)
  type(camera_t) :: cam
  integer(ci) :: cam_x, cam_y
  real(cf)  :: mvp(4,4), cam_pos_packed(4)

  status = vk_init(3440,1440, "Fortran Vulkan World" // c_null_char)
  if (status /= 0) error stop "Failed to initialized Vulkan backend"

  ! Request shared buffer from Vulkan and bind to a 2D Fortran array pointer
  raw_gpu_ptr = vk_allocate_heightmap(GRID_W, GRID_H)
  if (.not. c_associated(raw_gpu_ptr)) error stop "Failed to allocate GPU heightmap buffer"
  call c_f_pointer(raw_gpu_ptr, world, [GRID_W, GRID_H])

  ! Populate the heightmap directly in Fortran (values in [0.0, 1.0])
  call world_generate(world, GRID_W, GRID_H)

  cam_x = GRID_W/2
  cam_y = GRID_H/2
  cam%pos = [        &
    real(cam_x, cf), &
    real(cam_y, cf), &
    (world(cam_x, cam_y)%bedrock + world(cam_x,cam_y)%sediment + world(cam_x,cam_y)%water)*Z_SCALE + 10.0_cf]

  ! Main simulation loop
  do while (vk_should_close() == 0)
    !Update the camera
    call camera_update(cam, 0.0166_cf)
    call camera_get_mvp(cam, 3440.0_cf / 1440.0_cf, mvp)

    cam_pos_packed(1:3) = cam%pos
    cam_pos_packed(4)   = Z_SCALE
    call vk_draw_frame(mvp, cam_pos_packed, VIEW_RADIUS, cam%view_mode)
  end do

  call vk_cleanup()

end program world_sim
