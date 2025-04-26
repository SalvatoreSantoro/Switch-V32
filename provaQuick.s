.section .data
arr: .word 8, 3, 5, 7, 2, 4, 1, 6  # Example array
n: .word 8                      # Array length

.section .text
.globl _start
_start:
    la   a0, arr           # Load address of array into a0
    lw   a1, n             # Load array length into a1
    addi a1, a1, -1        # Set upper bound: n-1
    li   a2, 0             # Lower bound: 0
    call quicksort         # Call quicksort(arr, 0, n-1)

    # Exit the program
    li   a7, 10            # syscall number for exit
    ecall

# quicksort function (arr, low, high)
quicksort:
    # Input: a0 = array address, a1 = low index, a2 = high index
    bge  a1, a2, quicksort_done  # If low >= high, return

    # Partition step
    call partition
    mv   a3, a0             # Save partition index to a3

    # Recursive call: quicksort(arr, low, pivot-1)
    mv   a2, a3
    addi a2, a2, -1
    call quicksort

    # Recursive call: quicksort(arr, pivot+1, high)
    mv   a1, a3
    addi a1, a1, 1
    call quicksort

quicksort_done:
    ret

# partition function (arr, low, high)
partition:
    # Input: a0 = array address, a1 = low index, a2 = high index
    mv   t0, a1             # t0 = low
    mv   t1, a2             # t1 = high
    lw   t2, 0(a0)          # t2 = arr[low] (pivot value)
    addi t2, t2, 0          # Prepare for comparison
    addi t0, t0, 1          # t0 = low + 1 (start comparison after pivot)
    
partition_loop:
    bge  t0, t1, partition_end  # If low >= high, break

    # Move to the right until arr[t0] >= pivot
partition_left:
    lw   t3, 0(t0)          # t3 = arr[t0]
    bge  t3, t2, partition_left_done
    addi t0, t0, 1          # Increment low index
    j    partition_left

partition_left_done:
    # Move to the left until arr[t1] <= pivot
partition_right:
    lw   t4, 0(t1)          # t4 = arr[t1]
    ble  t4, t2, partition_right_done
    addi t1, t1, -1         # Decrement high index
    j    partition_right

partition_right_done:
    bge  t0, t1, partition_end

    # Swap arr[t0] and arr[t1]
    lw   t3, 0(t0)          # t3 = arr[t0]
    lw   t4, 0(t1)          # t4 = arr[t1]
    sw   t4, 0(t0)          # arr[t0] = t4
    sw   t3, 0(t1)          # arr[t1] = t3

    j    partition_loop

partition_end:
    # Swap pivot into the correct position
    lw   t3, 0(a1)          # t3 = arr[low] (pivot)
    lw   t4, 0(t1)          # t4 = arr[t1]
    sw   t4, 0(a1)          # arr[low] = t4
    sw   t3, 0(t1)          # arr[t1] = pivot
    mv   a0, t1             # Return partition index in a0
    ret
