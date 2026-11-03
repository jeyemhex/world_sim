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

    function vk_should_close() bind(C, name="vk_should_close")
      import :: c_int
      integer(c_int) :: vk_should_close
    end function vk_should_close

    subroutine vk_cleanup() bind(C, name="vk_cleanup")
    end subroutine vk_cleanup
  end interface

  integer(c_int) :: status

  status = vk_init(1280, 720, "Fortran Vulkan World" // c_null_char)
  if (status /= 0) error stop "Failed to initialized Vulkan backend"

  ! Main simulation loop
  do while (vk_should_close() == 0)

  end do

  call vk_cleanup()

end program world_sim
