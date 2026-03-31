global main
extern printf, socket, bind, recvfrom, sendto, htons, htonl
extern strncmp, strcmp, sscanf, strlen, strcpy

; Aprendí a declarar variables gracias a chat xd 
; https://chatgpt.com/share/69cc454b-f718-83e9-91f4-eeac7ef81edf

section .data
; mensajes de consola
mensaje_inicio  db "Broker UDP corriendo en puerto 5000",10,0
mensaje_recibio   db "Llego: %s",10,0
msg_newsub db "Nuevo sub al topic %s",10,0
msg_pub    db "Se envio a %d clientes",10,0

fmt_sub db "SUB %49s",0
fmt_pub db "PUB %49s %[^\n]",0
strSUB  db "SUB",0
strPUB  db "PUB",0

section .bss
sockfd      resd 1
buffer      resb 1024
topic       resb 50
message     resb 1024

server_addr resb 16
client_addr resb 16
client_len  resd 1

; arreglo donde guardamos subs
; cada sub ocupa 70 bytes:
; 16 bytes sockaddr + 50 topic + padding
subs        resb 7000
sub_count   resd 1

section .text

; calcula direccion del sub i
get_sub_addr:
    mov edx, 70
    mul edx
    add eax, subs
    ret

main:

; crear socket UDP -> socket(2,2,0)
    push 0
    push 2
    push 2
    call socket
    add esp,12
    mov [sockfd],eax

; llenar server_addr
    mov word [server_addr],2

    push 5000
    call htons
    add esp,4
    mov [server_addr+2],ax

    push 0
    call htonl
    add esp,4
    mov [server_addr+4],eax

; bind(socket,addr,16)
    push 16
    push server_addr
    push dword [sockfd]
    call bind
    add esp,12

    push mensaje_inicio
    call printf
    add esp,4

ciclo_main:

; recvfrom
    mov dword [client_len],16
    push client_len
    push client_addr
    push 0
    push 1023
    push buffer
    push dword [sockfd]
    call recvfrom
    add esp,24

    push buffer
    push mensaje_recibio
    call printf
    add esp,8

; ver si empieza por SUB
    push 3
    push strSUB
    push buffer
    call strncmp
    add esp,12
    cmp eax,0
    je handle_sub

; ver si empieza por PUB
    push 3
    push strPUB
    push buffer
    call strncmp
    add esp,12
    cmp eax,0
    je handle_pub

    jmp ciclo_main

; ---------------- SUB ----------------
handle_sub:

; sacar topic
    push topic
    push fmt_sub
    push buffer
    call sscanf
    add esp,12

; si hay mas de 100 subs ignoramos
    mov eax,[sub_count]
    cmp eax,100
    jge ciclo_main

; obtener direccion donde guardar
    call get_sub_addr
    mov edi,eax

; copiar sockaddr cliente
    mov esi,client_addr
    mov ecx,4
copiar_sock:
    mov eax,[esi]
    mov [edi],eax
    add esi,4
    add edi,4
    loop copiar_sock

; copiar topic
    push topic
    push edi
    call strcpy
    add esp,8

    inc dword [sub_count]

    push topic
    push msg_newsub
    call printf
    add esp,8

    jmp ciclo_main

; ---------------- PUB ----------------
handle_pub:

; obtener topic y mensaje
    push message
    push topic
    push fmt_pub
    push buffer
    call sscanf
    add esp,16

    mov ecx,[sub_count]
    xor ebx,ebx
    xor esi,esi

loop_subs:
    cmp esi,ecx
    jge fin_pub

    push esi
    push ecx

    mov eax,esi
    call get_sub_addr
    mov edi,eax

    lea eax,[edi+16]
    push topic
    push eax
    call strcmp
    add esp,8

    pop ecx
    pop esi

    cmp eax,0
    jne siguiente

; enviar mensaje
    push esi
    push ecx

    mov eax,esi
    call get_sub_addr
    mov edi,eax

    push message
    call strlen
    add esp,4
    mov edx,eax

    push 16
    push edi
    push 0
    push edx
    push message
    push dword [sockfd]
    call sendto
    add esp,24

    pop ecx
    pop esi

    inc ebx

siguiente:
    inc esi
    jmp loop_subs

fin_pub:
    push ebx
    push msg_pub
    call printf
    add esp,8

    jmp ciclo_main