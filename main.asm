; LCD pins
LCD_PORT equ P1
LCD_E equ P1.3
LCD_RS equ P1.2
	
; OneWire pin
PIN_1W equ  P3.3

; Heater relay pin
PIN_RELAY equ P3.7
	
; Keyboard pins
KEY_DEC equ P1.0
KEY_INC equ P1.1

; Timer consts for generic 8051 for 1 ms delay
INIT_TL0 equ 017h
INIT_TH0 equ 0FCh
	
; Display buffer start address
DISP_BUFFER equ 030h

org 0000h
	
sjmp main

text1:
db "T = ", 0
text2:
db ".", 0
	
main:
; I/O pins initialization
mov LCD_PORT, #0
clr LCD_E
clr LCD_RS
clr PIN_RELAY
setb KEY_DEC
setb KEY_INC

; Timer initialization
; Timer0 used for delay
orl TMOD, #00010001b
anl TMOD, #11111101b
mov P0, #0
mov P2, #0

; Display initialization
acall lcd_init

main_loop:
	acall read_sensor
	acall convert_int_temp
	acall to_ascii
	mov r2, 00h
	
	acall load_int_disp_buffer
	
	mov r0, #DISP_BUFFER
	mov a, #01h ; clear LCD
	clr LCD_RS
	acall lcd_send_byte
	mov dptr, #text1
	setb LCD_RS
	acall lcd_send_chars ; display constant text
	acall lcd_send_chars_ram ; display dynamic text
	mov dptr, #text2
	acall lcd_send_chars ; display constant text
	mov 00h, r2
	acall convert_frac_temp
	acall to_ascii

	acall load_frac_disp_buffer

	mov r0, #DISP_BUFFER
	acall lcd_send_chars_ram ; display dynamic text
	mov r0, #255
	delay:
		acall delay_1ms
		djnz r0, delay
	sjmp main_loop

delay_1ms:
	; load initial timer values
	mov TL0, #INIT_TL0
	mov TH0, #INIT_TH0
	; enable timer
	setb TR0
	; wait for overflow
	wait:
		jnb TF0, wait
	; disable timer
	clr TR0
	; clear overflow flag
	clr TF0
	ret

simple_delay:
	djnz r7, simple_delay
	ret

; TODO : hardware and software for strong 1W pullup
init_1W:
	setb PIN_1W
	mov r7, #48
	acall simple_delay

	clr PIN_1W

	mov r7, #180
	acall simple_delay
	mov r7, #180
	acall simple_delay

	setb PIN_1W

	mov r7, #84
	acall simple_delay

	jnb PIN_1W, init_error
	init_error:
		orl 06h, #1
		jmp init_end
	anl 06h, #0FEh
	init_end:
	mov r7, #20
	acall simple_delay
	ret

; result in accumulator
read_byte_1W:
	mov r6, #8
	clr a
	start_read:
		clr PIN_1W
		clr C
		rrc a
		setb PIN_1W
		jnb PIN_1W, read_end
		orl a, #080h
		read_end:
			mov r7, #30
			acall simple_delay
		djnz r6, start_read
	ret

; write byte from accumulator
write_byte_1W:
	mov r6, #8
	start_write:
		clr PIN_1W
		rrc a
		mov PIN_1W, C
		mov r7, #45
		acall simple_delay
		setb PIN_1W
		djnz r6, start_write
	mov r7, #30
	acall simple_delay
	ret

; read temperature from DS18B20 sensor
read_sensor:
	acall init_1W
	; TODO : handle error
	mov a, #0CCh ; skip ROM
	acall write_byte_1W
	mov a, #044h ; convert T
	acall write_byte_1W
	acall init_1W
	mov a, #0CCh ; skip ROM
	acall write_byte_1W
	mov a, #0BEh ; read Scratchpad
	acall write_byte_1W
	acall read_byte_1W
	mov r0, a
	acall read_byte_1W
	mov r1, a
	ret
	
lcd_send_byte:
	anl LCD_PORT, #0Fh
	mov r7, a
	anl a, #0F0h
	mov b, LCD_PORT
	anl b, #0Fh
	orl a, b
	mov LCD_PORT, a
	mov a, r7
	setb LCD_E
	
	acall delay_1ms

	clr LCD_E
	
	mov r7, #40
	lcd_cmd_wait:
		acall delay_1ms
		djnz r7, lcd_cmd_wait
	
	anl LCD_PORT, #0Fh
	mov r7, #4
	lcd_shift:
		clr c
		rlc a
		djnz r7, lcd_shift
	anl a, #0F0h
	mov b, LCD_PORT
	anl b, #0Fh
	orl a, b
	mov LCD_PORT, a
	setb LCD_E
		
	acall delay_1ms	
		
	clr LCD_E
	mov r7, #40
	lcd_cmd_wait2:
		acall delay_1ms
		djnz r7, lcd_cmd_wait2
	ret
		
lcd_init:
	clr LCD_RS
	mov a, #02h ; 4 bit mode
	acall lcd_send_byte
	mov a, #028h ; 5x7 chars
	acall lcd_send_byte
	mov a, #0Eh ; cursor on
	acall lcd_send_byte
	mov a, #01h ; clear
	acall lcd_send_byte
	mov a, #080h ; move cursor to first position
	acall lcd_send_byte
	ret
	
lcd_send_chars:
	setb LCD_RS
	lcd_send_chars_loop:
		clr a
		movc a, @a+dptr
		jz lcd_send_stop
		acall lcd_send_byte	
		inc dptr
		sjmp lcd_send_chars_loop
	lcd_send_stop:
	ret
	
lcd_send_chars_ram:
	setb LCD_RS
	lcd_send_chars_ram_loop:
		mov a, @r0
		jz lcd_send_ram_stop
		acall lcd_send_byte	
		inc r0
		sjmp lcd_send_chars_ram_loop
	lcd_send_ram_stop:
	ret
	
convert_int_temp:
	mov a, r1
	anl a, #080h
	clr PSW^1
	jnz conv_int_neg_temp
	sjmp conv_int_pos_temp
	conv_int_neg_temp:
	setb PSW^1 ; used to display '-'
	mov a, r0
	cpl a
	clr c
	inc a
	mov r0, a
	mov a, r1
	cpl a
	addc a, #0
	mov r1, a
	conv_int_pos_temp:
	mov a, r1
	mov r2, #4
	convert_int_temp_loop:
		clr c
		rlc a
		djnz r2, convert_int_temp_loop
	mov b, a
	mov a, r0
	mov r2, #4
	convert_int_temp_loop2:
		clr c
		rrc a
		djnz r2, convert_int_temp_loop2
	orl a, b
	ret
	
convert_frac_temp:
	mov a, r0
	anl a, #0Fh
	mov b, #6
	mul ab
	ret
	
to_ascii:
	mov b, #10
	div ab
	add a, #'0'
	mov r7, a
	mov a, b
	add a, #'0'
	mov b, a
	mov a, r7
	ret
	
load_int_disp_buffer:
	mov r0, #DISP_BUFFER
	jb PSW^1, disp_minus
	sjmp skip
	disp_minus:
	mov r3, a
	mov a, #'-'
	mov @r0, a
	inc r0
	mov a, r3
	skip:
	mov @r0, a
	inc r0
	mov @r0, b
	inc r0
	mov @r0, #0
	ret
	
load_frac_disp_buffer:
	mov r0, #DISP_BUFFER
	mov @r0, a
	inc r0
	mov @r0, b
	inc r0
	mov @r0, #0
	ret

; Debounces buttons and sets a to 1 if KEY_DEC is pressed
;                                 2 if KEY_INC is pressed
;                                 3 if both keys are pressed
get_keys:
	clr a
	jnb KEY_DEC, get_key_inc
	orl a, #1
	get_key_inc:
	jnb KEY_INC, get_key_end
	orl a, #2
	get_key_end:
	; wait for buttons to stabilize
	mov r0, #20
	get_key_delay:
		acall delay_1ms
	djnz r0, get_key_delay
	jb KEY_DEC, get_key_inc2
	anl a, #0FEh
	get_key_inc2:
	jb KEY_INC, get_key_end2
	anl a, #0FDh
	get_key_end2:
	ret
end
