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
  use, intrinsic :: iso_c_binding
  implicit none

  interface
    function vk_init(width, height, title) bind(C, name="vk_init")
      import :: c_int, c_char
      integer(c_int)                              :: vk_init
      integer(c_int),                  value      :: width, height
      character(c_char), dimension(*), intent(in) :: title
    end function vk_init

    function vk_allocate_heightmap(grid_w, grid_h) bind(C, name="vk_allocate_heightmap")
      import :: c_int, c_ptr
      type(c_ptr) :: vk_allocate_heightmap
      integer(c_int), value :: grid_h, grid_w
    end function vk_allocate_heightmap

    function vk_should_close() bind(C, name="vk_should_close")
      import :: c_int
      integer(c_int) :: vk_should_close
    end function vk_should_close

    subroutine vk_draw_frame() bind(C, name="vk_draw_frame")
    end subroutine vk_draw_frame

    subroutine vk_cleanup() bind(C, name="vk_cleanup")
    end subroutine vk_cleanup
  end interface

  integer(c_int), parameter :: GRID_W = 512, GRID_H = 512
  integer(c_int) :: status, ix, iy
  type(c_ptr) :: raw_gpu_ptr
  real(c_float), pointer :: heightmap(:,:)
  real(c_float) :: dx, dy, dist

  status = vk_init(1280, 720, "Fortran Vulkan World" // c_null_char)
  if (status /= 0) error stop "Failed to initialized Vulkan backend"

  ! Request shared buffer from Vulkan and bind to a 2D Fortran array pointer
  raw_gpu_ptr = vk_allocate_heightmap(GRID_W, GRID_H)
  if (.not. c_associated(raw_gpu_ptr)) error stop "Failed to allocate GPU heightmap buffer"
  call c_f_pointer(raw_gpu_ptr, heightmap, [GRID_W, GRID_H])

  ! Populate the heightmap directly in Fortran (values in [0.0, 1.0])
  do iy=1, GRID_H
    do ix=1, GRID_W
      dx = real(ix - GRID_W/2., c_float) / real(GRID_W/2., c_float)
      dy = real(iy - GRID_H/2., c_float) / real(GRID_H/2., c_float)
      dist = sqrt(dx*dx + dy*dy)
      heightmap(ix, iy) = 0.5_c_float + 0.45_c_float * cos(dist * 10.0_c_float)*exp(-dist * 1.5_c_float)
    end do
  end do

  ! Main simulation loop
  do while (vk_should_close() == 0)
    call vk_draw_frame()
  end do

  call vk_cleanup()

end program world_sim
