global main
extern printf, scanf, socket, sendto, htons, inet_addr, sprintf

section .data
prompt_topic db "Topic: ",0
prompt_msg   db "Mensaje: ",0
fmt_str db "%s",0
fmt_pub db "PUB %s %s",0
ip db "127.0.0.1",0

section .bss
sockfd resd 1
topic  resb 64
msg    resb 256
finalmsg resb 400
sockaddr resb 16

section .text

main:

; crear socket UDP  socket(2,2,0)
    push 0
    push 2
    push 2
    call socket
    add esp,12
    mov [sockfd],eax

; llenar sockaddr a mano
    mov word [sockaddr],2

; puerto 5000
    push 5000
    call htons
    add esp,4
    mov [sockaddr+2],ax

; ip 127.0.0.1
    push ip
    call inet_addr
    add esp,4
    mov [sockaddr+4],eax

; pedir topic solo una vez
    push prompt_topic
    call printf
    add esp,4

    push topic
    push fmt_str
    call scanf
    add esp,8

; loop infinito enviando mensajes
loop_envio:

; pedir mensaje cada vez
    push prompt_msg
    call printf
    add esp,4

    push msg
    push fmt_str
    call scanf
    add esp,8

; armar string final -> "PUB topic mensaje"
    push msg
    push topic
    push fmt_pub
    push finalmsg
    call sprintf
    add esp,16

; enviar
    push 16
    push sockaddr
    push 0
    push 400
    push finalmsg
    push dword [sockfd]
    call sendto
    add esp,24

    jmp loop_envio