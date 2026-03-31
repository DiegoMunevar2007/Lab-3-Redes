global main
extern printf, scanf, socket, connect, sendto, recvfrom, sprintf, htons, inet_addr

section .data
prompt_topic db "Topic a suscribirse: ",0
msg_subscribed db "Suscrito a %s",10,0
msg_received db "Llego mensaje: %s",10,0
fmt_str db "%s",0
fmt_sub db "SUB %s",0
ip db "127.0.0.1",0

section .bss
sockfd resd 1
topic resb 64
submsg resb 64
buffer resb 1024
sockaddr resb 16
fromaddr resb 16
fromlen resd 1

section .text

main:

; crear socket UDP
    push 0
    push 2
    push 2
    call socket
    add esp,12
    mov [sockfd],eax

; llenar sockaddr
    mov word [sockaddr],2

    push 5000
    call htons
    add esp,4
    mov [sockaddr+2],ax

    push ip
    call inet_addr
    add esp,4
    mov [sockaddr+4],eax

; connect al broker
    push 16
    push sockaddr
    push dword [sockfd]
    call connect
    add esp,12

; pedir topic
    push prompt_topic
    call printf
    add esp,4

    push topic
    push fmt_str
    call scanf
    add esp,8

; crear mensaje "SUB topic"
    push topic
    push fmt_sub
    push submsg
    call sprintf
    add esp,12

; enviar SUB al broker
    push 16
    push sockaddr
    push 0
    push 64
    push submsg
    push dword [sockfd]
    call sendto
    add esp,24

    push topic
    push msg_subscribed
    call printf
    add esp,8

; loop infinito esperando mensajes
recv_loop:

    mov dword [fromlen],16

    push fromlen
    push fromaddr
    push 0
    push 1024
    push buffer
    push dword [sockfd]
    call recvfrom
    add esp,24

    cmp eax,0
    jl recv_loop

; poner \0 al final del mensaje
    mov ebx,eax
    mov byte [buffer+ebx],0

    push buffer
    push msg_received
    call printf
    add esp,8

    jmp recv_loop