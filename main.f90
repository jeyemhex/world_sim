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

  integer(c_int), parameter :: GRID_W = 128, GRID_H = 72
  integer(c_int) :: status, ix, iy
  type(c_ptr) :: raw_gpu_ptr
  real(c_float) :: max_val, min_val
  real(c_float), pointer :: heightmap(:,:)

  integer :: iter

  status = vk_init(1280, 720, "Fortran Vulkan World" // c_null_char)
  if (status /= 0) error stop "Failed to initialized Vulkan backend"

  ! Request shared buffer from Vulkan and bind to a 2D Fortran array pointer
  raw_gpu_ptr = vk_allocate_heightmap(GRID_W, GRID_H)
  if (.not. c_associated(raw_gpu_ptr)) error stop "Failed to allocate GPU heightmap buffer"
  call c_f_pointer(raw_gpu_ptr, heightmap, [GRID_W, GRID_H])

  ! Populate the heightmap directly in Fortran (values in [0.0, 1.0])
  call random_number(heightmap)
  print *, max_val, min_val

  do iter=1, 25
    !$omp parallel do default(none) shared(heightmap) private(ix)
    do iy=2, GRID_H-1
      do ix=2+mod(iy,2), GRID_W-1, 2
        heightmap(ix, iy) = (2*heightmap(ix,iy) + 0.5*(heightmap(ix-1,iy) + heightmap(ix,iy-1) + heightmap(ix+1,iy) + heightmap(ix,iy+1)))/4
      end do
    end do
    !$omp parallel do default(none) shared(heightmap) private(ix)
    do iy=2, GRID_H-1
      do ix=2+mod(iy+1,2), GRID_W-1, 2
        heightmap(ix, iy) = (2*heightmap(ix,iy) + 0.5*(heightmap(ix-1,iy) + heightmap(ix,iy-1) + heightmap(ix+1,iy) + heightmap(ix,iy+1)))/4
      end do
    end do

    heightmap(:,1) = heightmap(:,GRID_H-1)
    heightmap(:,GRID_H) = heightmap(:,2)
    heightmap(1,:) = heightmap(GRID_W-1,:)
    heightmap(GRID_W,:) = heightmap(2,:)
    min_val = minval(heightmap)
    max_val = maxval(heightmap)
    heightmap = (heightmap - min_val) / (max_val - min_val)
    print *, "Finished iter ", iter, ", range = ", maxval(heightmap), minval(heightmap)
  end do

  ! Main simulation loop
  do while (vk_should_close() == 0)
    call vk_draw_frame()
  end do

  call vk_cleanup()

contains
  real function rand()
    call random_number(rand)
  end function rand
end program world_sim
