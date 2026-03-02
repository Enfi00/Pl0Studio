; Компіляція: nasm -f elf64 calc.asm -o calc.o
; Лінкування (через gcc): gcc calc.o driver.c -o app

extern printf
extern K            ; Змінна K визначена в C коді (або іншій одиниці трансляції)

section .data
    msg db 'Результат обчислень з asm: %d', 10, 0  ; 10 = \n (новий рядок)

section .text
    global calc

calc:
    ; Аргументи Linux System V ABI:
    ; 1-й (B) = RDI (int16_t) -> di
    ; 2-й (C) = RSI (int8_t)  -> sil (молодший байт rsi)
    ; 3-й (D) = RDX (int16_t) -> dx

    ; --- Пролог ---
    push rbx        ; Зберігаємо rbx (callee-saved) + вирівнюємо стек на 16 байт
                    ; (8 байт адреса повернення + 8 байт rbx = 16)

    ; ---- Обчислення 4*(B - C) ----
    movsx eax, di   ; B (16-bit) -> eax
    movsx ecx, sil  ; C (8-bit) -> ecx (Увага: sil замість dl)
    
    sub eax, ecx    ; eax = B - C
    shl eax, 2      ; eax = 4*(B - C)
    
    mov ebx, eax    ; Зберігаємо проміжний результат в ebx
                    ; (ebx зберігається між викликами функцій)

    ; ---- Обчислення D/4 з truncate toward zero ----
    movsx eax, dx   ; D (16-bit, 3-й аргумент в RDX) -> eax
    cdq             ; Розширення знаку eax -> edx:eax
    mov ecx, 4      ; Дільник
    idiv ecx        ; eax = D / 4
    
    ; ---- Додаємо все ----
    add ebx, eax    ; 4*(B - C) + D/4
    
    ; Додаємо K. Використовуємо [rel K] для PIC (Position Independent Code)
    mov eax, [rel K] 
    add ebx, eax    ; ebx = Final Result

    ; ---- Виклик printf ----
    ; printf(const char *format, ...)
    ; RDI = адреса рядка
    ; RSI = значення (наш результат)
    
    lea rdi, [rel msg] ; 1-й аргумент: формат
    mov esi, ebx       ; 2-й аргумент: число (результат)
    xor eax, eax       ; AL = 0 (кількість векторних регістрів для printf)
    
    call printf WRT ..plt  ; WRT ..plt потрібно для лінкування спільних бібліотек

    ; ---- Повернення результату ----
    mov eax, ebx    ; Повертаємо результат через EAX
    
    ; --- Епілог ---
    pop rbx         ; Відновлюємо rbx
    ret
