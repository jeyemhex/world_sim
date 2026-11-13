module world_mod
!==============================================================================#
! WORLD_MOD
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

  real(cf), parameter, public  :: SEA_LEVEL = 0.38_cf

  type, bind(C), public :: cell_t
    real(cf) :: bedrock
    real(cf) :: sediment
    real(cf) :: water
    real(cf) :: moisture
  end type cell_t

  public :: world_generate

contains

  function distance(a, b, bounds) result(d)
    real :: d
    integer a(2), b(2), bounds(2)

    real :: dx, dy

    dx = min( abs(a(1)-b(1)), &
              abs(a(1)-b(1) + bounds(1)), &
              abs(a(1)-b(1) - bounds(1)) )
              
    dy = min( abs(a(2)-b(2)), &
              abs(a(2)-b(2) + bounds(2)), &
              abs(a(2)-b(2) - bounds(2)) )
              

    d = dx**2 + dy**2

  end function distance
  subroutine world_generate(grid, nx, ny)
    type(cell_t), intent(inout) :: grid(nx, ny)
    integer,      intent(in)    :: nx, ny

    real               :: noise(nx, ny)
    integer            :: voronoi_mask(nx, ny)
    integer, parameter :: n_plates = 8
    integer            :: plate_centres(2,n_plates)
    integer :: i, ix, iy
    real :: min_dist

    grid(:,:)%bedrock  = 0.5_cf
    grid(:,:)%sediment = 0.0_cf
    grid(:,:)%water    = 0.0_cf
    grid(:,:)%moisture = 0.0_cf

    call random_number(noise(1:3, 1:n_plates))
    noise(1,1:n_plates) = noise(1,1:n_plates) * nx
    noise(2,1:n_plates) = noise(2,1:n_plates) * ny
    plate_centres = nint(noise(1:2, 1:n_plates))
    
    do i=1, n_plates
      voronoi_mask(plate_centres(1,i), plate_centres(2,i)) = i
    end do

    voronoi_mask = 0

    do iy=1, ny
      do ix=1, nx
        voronoi_mask(ix,iy) = 1
        min_dist = huge(1.0)
        do i=1, n_plates
          if (distance([ix,iy], plate_centres(:,i), [nx, ny]) < min_dist) then
            voronoi_mask(ix,iy) = i
            min_dist = distance([ix,iy], plate_centres(:,i), [nx, ny])
          end if
        end do

        grid(ix,iy)%bedrock = noise(3,voronoi_mask(ix,iy))
      end do
    end do

    where(grid%bedrock < SEA_LEVEL) grid%water = SEA_LEVEL - grid%bedrock



  end subroutine world_generate

  pure real(cf) function sample_periodic_bicubic(arr,px,py,x,y) result(val)
    real(cf), intent(in) :: arr(:,:)
    integer,  intent(in) :: px, py
    real(cf), intent(in) :: x, y

    integer  :: x0, y0, x1, y1
    real(cf) :: fx, fy, u, v, v0, v1

    x0 = modulo(floor(x), px) + 1
    y0 = modulo(floor(y), py) + 1
    x1 = modulo(x0, px) + 1
    y1 = modulo(y0, py) + 1

    fx = x - floor(x)
    fy = y - floor(y)

    ! Quintic perlin smootherstep: 6t^5 + 15t^4 + 10t^3 (eliminates gridline artifacts)
    u = fx*fx*fx * (fx * (fx*6.0_cf - 15.0_cf) + 10.0_cf)
    v = fy*fy*fy * (fy * (fy*6.0_cf - 15.0_cf) + 10.0_cf)

    v0  = (1.0_cf-u)*arr(x0, y0) + u*arr(x1,y0)
    v1  = (1.0_cf-u)*arr(x0, y1) + u*arr(x1,y1)
    val = (1.0_cf-v)*v0 + v*v1

  end function sample_periodic_bicubic

end module world_mod
