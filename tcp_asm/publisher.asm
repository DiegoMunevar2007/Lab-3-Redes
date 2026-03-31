global main

extern printf
extern scanf
extern socket
extern connect
extern inet_addr
extern htons
extern send
extern close
extern strcmp
extern snprintf
extern getchar
extern exit

SECTION .data
    ; mensajes que se imprimen en pantalla
    msg_socket_error db "Error creando socket", 10, 0
    msg_connect_error db "Error : Connect Failed", 10, 0
    msg_topic db "Ingrese topic: ", 0
    msg_input db "Ingrese mensaje (o 'SALIR' para desconectar): ", 0
    msg_sent db "Mensaje enviado", 10, 0

    ; formatos para scanf / snprintf
    fmt_str db "%s", 0
    fmt_line db "%[^",10,"]", 0
    fmt_pub db "PUB %s %s", 0

    salir_str db "SALIR", 0
    ip_str db "127.0.0.1", 0

SECTION .bss
    ; variables sin inicializar
    sockfd resd 1
    topic resb 50
    message resb 1024
    final_msg resb 1200
    server_addr resb 16   ; estructura sockaddr_in

SECTION .text

main:
    push ebp
    mov ebp, esp

    ; limpiar server_addr (como memset a 0)
    mov edi, server_addr
    mov ecx, 16
    xor eax, eax
    rep stosb

    ; server_addr.sin_family = AF_INET (2)
    mov word [server_addr], 2

    ; server_addr.sin_port = htons(8000)
    push 8000
    call htons
    add esp, 4
    mov [server_addr + 2], ax

    ; server_addr.sin_addr = inet_addr("127.0.0.1")
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
    push msg_topic
    call printf
    add esp, 4

    ; leer topic con scanf
    push topic
    push fmt_str
    call scanf
    add esp, 8

    ; limpiar salto de linea del buffer
    call getchar

loop_start:

    ; pedir mensaje al usuario
    push msg_input
    call printf
    add esp, 4

    ; leer linea completa incluyendo espacios
    push message
    push fmt_line
    call scanf
    add esp, 8

    ; limpiar salto de linea
    call getchar

    ; comparar si el usuario escribió "SALIR"
    push salir_str
    push message
    call strcmp
    add esp, 8

    cmp eax, 0
    je end_program

    ; construir mensaje final "PUB topic mensaje"
    push message
    push topic
    push fmt_pub
    push 1200
    push final_msg
    call snprintf
    add esp, 20

    ; snprintf devuelve la longitud del string en eax
    mov edx, eax

    ; enviar mensaje por socket TCP
    push 0
    push edx
    push final_msg
    push dword [sockfd]
    call send
    add esp, 16

    cmp eax, 0
    jl send_error

    ; confirmar envio
    push msg_sent
    call printf
    add esp, 4

    jmp loop_start

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

send_error:
    ; si falla send, terminar programa
    jmp end_program

end_program:
    ; cerrar socket antes de salir
    push dword [sockfd]
    call close
    add esp, 4
    xor eax, eax

cleanup:
    ; restaurar stack y salir
    mov esp, ebp
    pop ebp
    ret