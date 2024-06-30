;;;;;;;;
;; For use with 86Box's ISADebugger
;;
%macro out_debug_left 1
  mov dx, 7Ah
  mov al, DEBUG
  out dx, al
  mov dx, 7Bh  
  mov al, %1 
  out dx, al 
%endmacro

%macro out_debug_right 1
  mov dx, 7Ah
  mov al, DEBUG - 2 
  out dx, al
  mov dx, 7Bh  
  mov al, %1 
  out dx, al
%endmacro

%macro out_debug 1
  push bx
  mov bx, %1

  push dx
  push ax

  out_debug_left bh
  out_debug_right bl
  
  pop ax
  pop dx
  pop bx
%endmacro


