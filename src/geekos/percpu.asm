%macro Get_Current_Thread_To_EAX 0
    Mov_EAX_Current_Thread_PTR
    mov	eax, [gs+4]              
%endmacro
%macro Set_Current_Thread_From_EBX 0
    Mov_EAX_Current_Thread_PTR
    mov	[gs+4], ebx
%endmacro
%macro Push_Current_Thread_PTR 0
    push	dword [gs+4]
%endmacro
