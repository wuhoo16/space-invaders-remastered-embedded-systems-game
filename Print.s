; Print.s
; Student names: Jin Lee (jl67888), Andrew Wu (amw5468)
; Last modification date: 4/1/19
; Runs on LM4F120 or TM4C123
; EE319K lab 7 device driver for any LCD
;
; As part of Lab 7, students need to implement these LCD_OutDec and LCD_OutFix
; This driver assumes two low-level LCD functions
; ST7735_OutChar   outputs a single 8-bit ASCII character
; ST7735_OutString outputs a null-terminated string 

;	LCD_OutDec
; 1) binding local variables
quotient	EQU 0	; uint32_t quotient
remainder 	EQU 4	; uint8_t remainder
;	LCD_OutFix
num			EQU 4
    IMPORT   ST7735_OutChar
    IMPORT   ST7735_OutString
    EXPORT   LCD_OutDec
    EXPORT   LCD_OutFix

    AREA    |.text|, CODE, READONLY, ALIGN=2
    THUMB
;	PRESERVE8
outOfRange DCB 0x2A, 0x2E, 0x2A, 0x2A, 0x2A, 0x00	; null-terminated string "*.***" for inputs >9999

;-----------------------LCD_OutDec-----------------------
; Output a 32-bit number in unsigned decimal format
; Input: R0 (call by value) 32-bit unsigned number
; Output: none
; Invariables: This function must not permanently modify registers R4 to R11
;----------C Implementation----------
;	void LCD_OutDec(uint32_t num){
;		uint32_t quotient = num/10;		// local variable
;		uint8_t remainder = num%10;		// local variable
;		if(quotient){					// print MSB first
;			LCD_OutDec(quotient);
;		}
;		ST7735_OutChar(remainder+'0');	// print ASCII
;	}
LCD_OutDec
	PUSH {R11, LR}
	SUB	SP, #8					; 2) allocation
	MOV	R11, SP					; R11 is frame pointer
	MOV	R1, #10
	UDIV R2, R0, R1				; R2 = num/10
	STR R2, [R11, #quotient]	; uint32_t quotient = num/10
	MUL R2, R1					; R2= 10*trunc(num/10)
	SUB R2, R0, R2				; R2 = num%10
	STRB R2, [R11, #remainder]	; remainder = num%10
	LDR R0, [R11, #quotient]	; 3) access
	CMP R0, #0					; print MSB first
	BEQ print
	BL LCD_OutDec				; LCD_OutDec(quotient)
print
	LDRB R0, [R11, #remainder]
	ADD R0, #0x30				; R0 = remainder + '0'
	PUSH {R0-R3}
	BL ST7735_OutChar;			; print ASCII
	POP {R0-R3}
	ADD SP, #8					; 4) deallocation
    POP {R11, PC}
;* * * * * * * * End of LCD_OutDec * * * * * * * *

; -----------------------LCD _OutFix----------------------
; Output characters to LCD display in fixed-point format
; unsigned decimal, resolution 0.001, range 0.000 to 9.999
; Inputs:  R0 is an unsigned 32-bit number
; Outputs: none
; E.g., R0=0,    then output "0.000 "
;       R0=3,    then output "0.003 "
;       R0=89,   then output "0.089 "
;       R0=123,  then output "0.123 "
;       R0=9999, then output "9.999 "
;       R0>9999, then output "*.*** "
; Invariables: This function must not permanently modify registers R4 to R11
;----------C Implementation----------
;	char outOfRange[6]={'*','.','*','*','*', 0x0}	// null-terminated string for inputs > 9999 (private global variable)
;	char inputASCII[4];								// buffer for print output operation (local variable)
;	void LCD_OutFix(uint32_t input){uint32_t num = input;	// local variable?
;		if(num > 9999){						// print "*.*** " if input is out of range
;			ST7735_OutString(*outOfRange);
;		}else{
;			for(int i = 0; i<4; i++){		// load inputASCII[] with ASCII digits in reverse order
;				inputASCII[i] = num%10 + '0';
;				num /= 10;
;			}
;			for(int i = 3; i>=0; i--){		// output the ASCII digits in fixed-point format
;				ST7735_OutChar(inputASCII[i];
;				if(i == 3){
;					ST7735_OutChar('.');	// output '.' after first character
;				}
;			}
;		}
;		return;
;	}
LCD_OutFix
	PUSH {R11, LR}
	SUB SP, #8					; 1) allocation
	MOV R11, SP					; R11 is frame pointer
	STR R0, [R11, #num]			; uint32_t num = input
	MOV R1, #9999
	CMP R0, R1					; if num > 9999,
	BLS storeNum				; print "*.***"
	MOV R0, #0x2A
	PUSH {R0-R3}
	BL ST7735_OutChar
	POP {R0-R3}
	MOV R0, #0x2E
	PUSH {R0-R3}
	BL ST7735_OutChar
	POP {R0-R3}
	MOV R0, #0x2A
	PUSH {R0-R3}
	BL ST7735_OutChar
	POP {R0-R3}
	PUSH {R0-R3}
	BL ST7735_OutChar
	POP {R0-R3}
	PUSH {R0-R3}
	BL ST7735_OutChar
	POP {R0-R3}
	B done						; and return
storeNum
	MOV R1, #0					; R1 is loop counter
storeLoop
	CMP R1, #4
	BHS printNum
	LDR R0, [R11, #num]			; R0 = num
	MOV R2, #10
	UDIV R3, R0, R2					; R3 = num/10
	STR R3, [R11, #num]			; update num for next iteration (num /= 10)
	MUL R3, R2					; R3= 10*trunc(num/10)
	SUB R0, R0, R3				; R0 = num%10
	ADD R0, #0x30				; R0 = num%10 + '0'
	STRB R0, [R11, R1]			; inputASCII[i] = num%10 + '0'
	ADD R1, #1					; next character (i++)
	B storeLoop
printNum
	MOV R1, #3					; R1 is loop counter
printLoop
	CMP R1, #0
	BLT done
	LDRB R0, [R11, R1]			; R0 = inputASCII[i]
	PUSH {R0-R3}
	BL ST7735_OutChar
	POP {R0-R3}
	CMP R1, #3					; output '.' after first character
	BNE notFirst
	MOV R0, #0x2E
	PUSH {R0-R3}
	BL ST7735_OutChar
	POP {R0-R3}
notFirst
	SUB R1, #1
	B printLoop
done
	ADD SP, #8
	POP {R11, PC}
 
     ALIGN
;* * * * * * * * End of LCD_OutFix * * * * * * * *

     ALIGN                           ; make sure the end of this section is aligned
     END                             ; end of file
