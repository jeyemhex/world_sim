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

  subroutine world_generate(grid, nx, ny)
    type(cell_t), intent(inout) :: grid(nx, ny)
    integer,      intent(in)    :: nx, ny

    integer :: ix, iy, oct, period_x, period_y
    real(cf) :: h, amp, freq_x, freq_y, total_amp, n_val
    real(cf) :: min_h, max_h, total_h, h_range
    real(cf), allocatable :: coarse(:,:)

    allocate(coarse(nx, ny))

    ! Initialize heights to zero
    grid%bedrock  = 0.0_cf
    grid%sediment = 0.0_cf
    grid%water    = 0.0_cf
    grid%moisture = 0.0_cf

    amp = 1.0_cf
    total_amp = 0.0_cf

    ! Sum 7 octaves of periodic noise (periods 4 - 128)
    do oct = 1, 6
      period_x = 2**(oct + 1)
      period_y = max(2, int(real(period_x)*real(ny)/real(nx)))

      call random_number(coarse(1:period_x, 1:period_y))

      do iy=1, ny
        do ix = 1, nx
          freq_x = real(ix-1, cf) * real(period_x, cf) / real(nx, cf)
          freq_y = real(iy-1, cf) * real(period_y, cf) / real(ny, cf)

          n_val = sample_periodic_bicubic(coarse, period_x, period_y, freq_x, freq_y)

          ! Mix smooth hils with sharp mountain ridges
          if (oct >= 3) then
            n_val = 1.0_cf - abs(2.0_cf*n_val - 1.0_cf)
            n_val = n_val * n_val
          end if

          grid(ix, iy)%bedrock = grid(ix,iy)%bedrock + amp*n_val
        end do
      end do

      total_amp = total_amp + amp
      amp = amp * 0.5_cf
    end do

    deallocate(coarse)

    ! Normalise bedrock to [0,1] and apply continental shelf curve 
    min_h = minval(grid%bedrock)
    max_h = maxval(grid%bedrock)
    h_range = max_h-min_h
    do iy=1, ny
      do ix=1, nx
        h = grid(ix,iy)%bedrock - min_h / h_range

        ! slightly flatten ocean basins and steepend mountain peaks
        h = h ** 1.25_cf

        ! Deposit a thin layer of loose sediment in the valleys/lowlands
        grid(ix,iy)%sediment = 0.05_cf * (1.0_cf - h)
        grid(ix,iy)%bedrock = h - grid(ix,iy)%sediment

        total_h = grid(ix,iy)%bedrock + grid(ix,iy)%sediment
        if (total_h < SEA_LEVEL) then
          grid(ix,iy)%water = SEA_LEVEL - total_h
        else
          grid(ix,iy)%water = 0.0_cf
        end if
      end do
    end do

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
