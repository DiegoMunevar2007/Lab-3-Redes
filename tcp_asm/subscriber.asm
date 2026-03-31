global main

extern printf
extern scanf
extern socket
extern connect
extern inet_addr
extern htons
extern send
extern recv
extern close
extern strlen
extern sprintf
extern exit

SECTION .data
    ; mensajes que se imprimen en consola
    msg_socket_error db "Error creando socket", 10, 0
    msg_connect_error db "Error : Connect Failed", 10, 0
    msg_prompt db "Ingrese topic a suscribirse: ", 0
    msg_subscribed db "Suscrito a %s", 10, 0
    msg_recv db "Mensaje recibido: %s", 10, 0
    msg_error db "Conexion cerrada o error", 10, 0

    fmt_str db "%s", 0
    fmt_sub db "SUB %s", 0

    ; ip del servidor
    ip_str db "127.0.0.1", 0

SECTION .bss
    ; variables sin inicializar
    sockfd resd 1
    topic resb 50
    buffer resb 1024
    sub_msg resb 64
    server_addr resb 16

SECTION .text

main:
    push ebp
    mov ebp, esp

    ; poner en 0 la estructura sockaddr (como memset)
    mov edi, server_addr
    mov ecx, 16
    xor eax, eax
    rep stosb

    ; sin_family = AF_INET (2)
    mov word [server_addr], 2

    ; sin_port = htons(8000)
    push 8000
    call htons
    add esp, 4
    mov [server_addr + 2], ax

    ; sin_addr = inet_addr("127.0.0.1")
    push ip_str
    call inet_addr
    add esp, 4
    mov [server_addr + 4], eax

    ; crear socket TCP -> socket(AF_INET, SOCK_STREAM, 0)
    push 0
    push 1
    push 2
    call socket
    add esp, 12

    mov [sockfd], eax
    cmp eax, 0
    jl socket_error

    ; conectar al servidor
    push 16
    push server_addr
    push dword [sockfd]
    call connect
    add esp, 12

    cmp eax, 0
    jl connect_error

    ; pedir topic al usuario
    push msg_prompt
    call printf
    add esp, 4

    ; leer topic
    push topic
    push fmt_str
    call scanf
    add esp, 8

    ; crear mensaje SUB con sprintf -> "SUB topic"
    push topic
    push fmt_sub
    push sub_msg
    call sprintf
    add esp, 12

    ; calcular longitud del mensaje SUB
    push sub_msg
    call strlen
    add esp, 4
    mov edx, eax

    ; enviar SUB al servidor
    push 0
    push edx
    push sub_msg
    push dword [sockfd]
    call send
    add esp, 16

    ; confirmar suscripción
    push topic
    push msg_subscribed
    call printf
    add esp, 8

recv_loop:

    ; esperar mensajes del servidor
    push 0
    push 1023
    push buffer
    push dword [sockfd]
    call recv
    add esp, 16

    ; si recv devuelve <=0 hubo error o se cerró conexión
    cmp eax, 0
    jle recv_error

    ; poner terminador de string al final del mensaje recibido
    mov ebx, buffer
    add ebx, eax
    mov byte [ebx], 0

    ; imprimir mensaje recibido
    push buffer
    push msg_recv
    call printf
    add esp, 8

    jmp recv_loop

recv_error:
    ; mensaje de error de conexión
    push msg_error
    call printf
    add esp, 4
    jmp cleanup

socket_error:
    ; error creando socket
    push msg_socket_error
    call printf
    add esp, 4
    mov eax, 1
    jmp cleanup

connect_error:
    ; error conectando al servidor
    push msg_connect_error
    call printf
    add esp, 4
    push 0
    call exit

cleanup:
    ; cerrar socket antes de salir
    push dword [sockfd]
    call close
    add esp, 4

    xor eax, eax
    mov esp, ebp
    pop ebp
    ret