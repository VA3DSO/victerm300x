#pragma charmap(147, 147)
#pragma charmap(17, 17)
#include <cbm.h>
#include <peekpoke.h>
#include <vic20.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void print(char* str);
void put_char(char ch);
char kb_hit(void);
char getkey(void);
char get(void);
void cursor_on(void);
void cursor_off(void);
void display_petscii_ascii(void);
void beep(void);
void download(void);
void upload(void);
void input(void);
char errorcheck(void);
void enjoythesilence(void);
void clear(char*);
void pad(char*);
void clearbuffer(void);
char inbyte(char, char*);
void outbyte(char ch);
int __fastcall__ rs232_read(unsigned char lfn, unsigned char* buffer, unsigned int size);

char f[] = {
      0,   0,   0, 137,   0,   0,   0,   0,  20,   0,   0,   0, 147,  13,   0,   0,
    146, 134,   0, 138,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
     48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
     64, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
    208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218,  91,  47,  93,  94, 164,
     39,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
     80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91, 221,  93, 186,   0
};

#define BUFFER_SIZE 48
#define COLOR_COMBOS 5

#define TRUE    1
#define FALSE   0
#define SUCCESS 0
#define ERROR   1
#define OFF     0
#define ON      1
#define F1      133
#define F3      134
#define F5      135
#define F7      136
#define F2      137
#define F4      138
#define F6      139
#define F8      140
#define BUFFER  0xA000
#define SC      0x900F

#define OK      0
#define TIMEOUT 1
#define CANCEL  2

#define SOH     1
#define EOT     4
#define ACK     6
#define NAK     21
#define CAN     24
#define PAD     0

#define JCH     160
#define JCM     161
#define JCL     162

#define MAXRETRIES 25

char ch;
char *open_string = "x";
char buffer[BUFFER_SIZE];
int len;
int i;
char is_petscii;
char is_start;
char color_combo;
char border_background[] = {27, 8, 8, 25, 238};
char foreground[] = {31, 30, 5, 144, 144};
char foreground_mem[] = {6, 5, 1, 0, 0};
char baud[] = {6, 7, 8}; // 300, 600, 1200
char I[33];

void main(void) {
    is_start = 1;
    is_petscii = 0;
    color_combo = 0;
    POKE(0x900E, 0);
    for (;;) {
        cursor_off();
        print("\223\016\010Choose modem speed:\n\n"
            "1-  300 BAUD (8n1)\n"
            "2-  600 BAUD (8n1)\n"
            "3- 1200 BAUD (8n1)\n"
        );
        do {
            ch = getkey();
        } while (ch < '1' || ch > '3');
        ch = ch - '1';
        open_string[0] = baud[ch];
        display_petscii_ascii();
        cursor_on();
        cbm_close(1);
        cbm_open(1,2,3,open_string);
        if (is_start) {
            is_start = 0;
            buffer[0] = '\n';
            cbm_write(1, buffer, 1);
        }
        for (;;) {
            ch = get();
            if (ch == 133)  { // F1
                is_petscii = !is_petscii;
                display_petscii_ascii();
            } else if (ch == 137) { // F2
                download();
            } else if (ch == 134) { // F3
                color_combo = (color_combo + 1) % COLOR_COMBOS;
                POKE(0x900F, border_background[color_combo]);
                put_char(foreground[color_combo]);
                #ifdef EXP8K
                for (i=0x9400; i<=0x95F9; ++i) POKE(i, foreground_mem[color_combo]);
                #else
                for (i=0x9600; i<=0x97F9; ++i) POKE(i, foreground_mem[color_combo]);
                #endif
            } else if (ch == 138) { // F4
                upload();
            } else if (ch == 135) { // F5
                break;
            } else if (ch == 136) { // F7
                cursor_off();
                put_char(foreground[color_combo]);
                print("\223\021\021\021\021\021\021\021\021\021\021"
                      " EXIT: ARE YOU SURE?");
                cursor_on();
                ch = getkey();
                if (ch == 'Y' || ch == 'y') asm("brk");
                display_petscii_ascii();
            } else if (ch != 0) {
                buffer[0] = is_petscii ? ch :
                    ch == 20 ? 8 :
                    (ch >= 65 && ch <= 90) ? ch+32 :
                    (ch >=193 && ch <=218) ? ch-128:
                    ch
                ;
                cbm_write(1, buffer, 1);
            }
            len = rs232_read(1, buffer, BUFFER_SIZE);
            if (len > 0) cursor_off();
            for (i=0; i<len; ++i) {
                if (buffer[i] == 7) beep();
                put_char(is_petscii ? buffer[i]: f[buffer[i]%128]);
                POKE(212, 0);
                POKE(216, 0);
            }
            if (PEEK(204) != 0) cursor_on();
        }
    }
}

void display_petscii_ascii(void) {
    cursor_off();
    put_char(foreground[color_combo]);
    print("\223\022VIC Terminal          \222"
        "\n"
        "F1-PET/ASCII F3-COLOR\n"
        "F5-SET BAUD  F7-EXIT\n"
        "F2-DOWNLOAD  F4-UPLD\n"
        "\300\300\300\300\300\300\300\300\300\300\300\300\300\300\300\300\300\300\300\300\300\300");
    print(is_petscii
        ? "* Charset: PETSCII\n\n"
        : "* Charset: ASCII\n\n"
    );
}

int __fastcall__ rs232_read(unsigned char lfn, unsigned char* buffer, unsigned int size)
{
    static unsigned int bytesread;
    if (cbm_k_chkin(lfn)) return -1;
    bytesread = 0;

    while (PEEK(667)!=PEEK(668) && bytesread<size)
        buffer[bytesread++] = cbm_k_getin();

    cbm_k_clrch();
    return bytesread;
}

void put_char(char ch) {
    __A__ = ch;
    asm("jsr $ffd2");
}

char get(void) {
    if (kb_hit()) {
        asm("jsr $e5cf");
        return __A__;
    }
    return 0;
}

char getkey(void) {
    while (1) {
        if (kb_hit()) {
            asm("jsr $e5cf");
            return __A__;
        }
    }
}

void print(char *str) {
    while (*str) {
        __A__ = *str++;
        asm("jsr $ffd2");
    }
}

char kb_hit(void) {
    asm("ldx #$00");
    asm("lda $c6");
    asm("beq %g", out);
    asm("lda #$01");
out:
    return __A__;
}

void cursor_on(void) {
    asm("ldy #$00");
    asm("sty $cc");
}

void cursor_off(void) {
    asm("ldy $cc");
    asm("bne %g", exitloop);
    asm("ldy #$01");
    asm("sty $cd");
    loop:
    asm("ldy $cf");
    asm("bne %g", loop);
    asm("ldy #$ff");
    asm("sty $cc");
    exitloop:
    return;
}

void beep(void) {
    static unsigned long j;
    POKE(0x900E, 15);
    POKE(0x900D, 0);
    POKE(0x900C, 230);
    for (j=0; j<1000; ++j) asm("nop");
    POKE(0x900E, 0);
}

void download(void) {

    char ftype, packet[131], filename[20], ch, i, m, e, blk, chk, checksum, badbytes;
    char receiving = TRUE, p = 1, retries = 0, status = OK;

    print("\n\nXmodem Download\n\n");

    print("Enter filename:\n>");
    input();

    if (strlen(I) > 0) {

        strcpy(filename, I);

        print("\nType (P/S/U)?\n>");
        do {
            ch = tolower(getkey());
        } while ((ch != 'p') && (ch != 's') && (ch != 'u'));

        sprintf(filename, "%s,%c,w", filename, ftype);

        print("\n\nDownloading... \n\n");

        cbm_open(15, 8, 15, "");
        cbm_open(3, 8, 3, filename);

        e = errorcheck();

        if (e == 0) {

            /* send NAK to initiate transfer */
            outbyte(NAK);

            do {

                /* get control byte */
                ch = inbyte('S', &status);

                if ((ch == 1) && (status == OK)) {

                    /* SOH received! */

                    /* get packet! */
                    sprintf(I, "\n\nBlock #%i:", p); print(I);
                    pad(packet);
                    badbytes = 0;
                    m = 0;

                    for (i = 0; i < 131; i++) {
                        ch = inbyte('S', &status);
                        if (status == OK) {
                            packet[i] = ch;
                            if ((i > 1) && (i < 130)) {
                                m = m + ch;
                            }
                            print("+");
                        } else {
                            print(".");
                            badbytes++;
                        }
                    }

                    if (badbytes == 0) {

                        blk = packet[0];
                        chk = packet[1];
                        checksum = packet[130];

                        /* validate packet */
                        if ((blk == p) && (255 - chk == blk) && (checksum == m)) {

                            /* its good! save it! */
                            for (i = 2; i < 130; ++i) {
                                cbm_k_ckout(3);
                                cbm_k_bsout(packet[i]);
                                cbm_k_clrch();
                            }
                            print("\272");
                            p++;
                            outbyte(ACK);

                        } else {

                            badbytes = 1;
                        }

                    }

                    if (badbytes > 0) {

                        /* bad packet! */
                        print("X");
                        enjoythesilence();
                        outbyte(NAK);

                        retries++;

                        if (retries > MAXRETRIES) {
                            status = CANCEL;
                            print("C");
                            outbyte(CAN);
                            outbyte(CAN);
                            outbyte(CAN);
                            receiving = FALSE;
                            break;
                        }
                    }


                } else if ((ch == 4) && (status == OK)) {

                    /* EOT received! */
                    print("E");
                    outbyte(ACK);
                    receiving = FALSE;

                } else {

                    status = CANCEL;
                    print("C");
                    outbyte(CAN);
                    outbyte(CAN);
                    outbyte(CAN);
                    receiving = FALSE;
                    break;

                }


            } while (receiving == TRUE);

            if (status == OK) {
                print("\n\nSUCCCES!\n");
            } else {
                print("\n\nFAILURE!\n");
            }

        }

        cbm_close(3);
        cbm_close(15);

    } else {

        print("\n\nABORTED!\n\n");
    }

    beep();

}

void upload(void) {

    /* UPLOAD ! */

    char ftype, packet[128], ch, i, j, m, e, st, t;
    char filename[20], sending = TRUE, p = 1, retries = 0, status = OK, retry = FALSE;

    print("\n\nXmodem Upload\n\n");

    print("Enter filename:\n>");
    input();

    if (strlen(I) > 0) {

        sprintf(filename, "%s,r", I, ftype);

        print("\n\nUploading... \n\n");

        print("Waiting for server..\n");

        cbm_open(15, 8, 15, "");
        cbm_open(3, 8, 3, filename);

        e = errorcheck();

        if (e == 0) {

            for (t = 0; t < MAXRETRIES; t++) {

                /* wait for NAK from remote... */
                ch = inbyte('L', &status);

                if (ch == NAK) {
                    break;
                }

            }

            if (ch == NAK) {

                /* lets go! */
                do {

                    /* clear buffer */
                    clearbuffer();

                    /* header */
                    outbyte(SOH);
                    outbyte(p);
                    outbyte(255 - p);

                    sprintf(I, "\n\nBlock #%i:", p); print(I);

                    if (retry == FALSE) {

                        m = 0;
                        pad(packet);

                        /* get bytes from disk */
                        for (i = 0; i < 128; i++) {
                            cbm_k_chkin(3);
                            ch = cbm_k_basin();
                            cbm_k_clrch();
                            st = cbm_k_readst();

                            if (st == 0) {
                                outbyte(ch);
                                print("+");
                                packet[i] = ch;
                                m = m + ch;
                            } else if (st == 64) {
                                /* end of file */
                                for (j = i; j < 128; j++) {
                                    print("+");
                                    outbyte(PAD);
                                    m = m + PAD;
                                }
                                i = 128;
                                break;
                            } else {
                                e = errorcheck();
                                status = CANCEL;
                                print("C");
                                outbyte(CAN);
                                outbyte(CAN);
                                outbyte(CAN);
                                sending = FALSE;
                                break;
                            }
                        }

                        outbyte(m);         /* checksum! */

                    } else {

                        m = 0;

                        /* re-send the packet */
                        for (i = 0; i < 128; i++) {
                            outbyte(packet[i]);
                            m = m + packet[i];
                            print("+");
                        }

                        outbyte(m);         /* checksum! */

                        retry = FALSE;

                    }

                    if ((st == 0) || (st ==64)) {

                        ch = inbyte('S', &status);

                        if (ch == ACK) {

                            print("\272");
                            p++;

                        } else if (ch == NAK) {

                            print("X");
                            retries++;

                            enjoythesilence();

                            if (retries > MAXRETRIES) {
                                status = CANCEL;
                                print("C");
                                outbyte(CAN);
                                outbyte(CAN);
                                outbyte(CAN);
                                sending = FALSE;
                                break;
                            }

                        } else if (ch == CAN) {
                            print("C");
                            sending = FALSE;
                            break;
                        }
                    }

                    if (st == 64) {

                        /* we are done! */

                        outbyte(EOT);

                        ch = inbyte('S', &status);

                        if (ch == NAK) {
                            /* send EOT again? */
                            outbyte(EOT);
                            ch = inbyte('S', &status);
                        }

                        if (ch == ACK) {
                            print("E");
                            print("\n\nSUCCESS!\n");
                            outbyte(ACK);
                        } else {
                            print("C");
                            print("\n\nFAILURE!\n");
                            outbyte(CAN);
                        }

                        sending = FALSE;

                    }

                } while (sending == TRUE);

            } else {

                print("\n\nABORTED!");

            }

        }

        cbm_close(3);
        cbm_close(15);

    } else {

        print("\n\nABORTED!\n\n");

    }

    beep();

}

void input(void) {

    char ch;
    char i = 0, done = FALSE;

    clear(I);

    do {

        cursor_on();

        do {

            /* XLOCAL */
            ch = cbm_k_getin();

        } while(ch == 0);

        cursor_off();

        cbm_k_bsout(ch);

        if (ch == 13) {
            done = TRUE;
            break;
        } else if (ch == 20) {
            if (i > 0) {
                I[i] = '\0';
                i--;
            }
        } else {
            if (i < 40) {
                I[i] = ch;
                i++;
            }
        }

    } while (done == FALSE);

}

char errorcheck() {

    /* assumes channel 15 is already open */

    char data[32];
    char err[3];
    int e = 0;

    cbm_read(15, data, 32);

    if ((data[0] != '0') || (data[1] != '0')) {
        print("\022ERROR: ");
        print(data);
        print("\222");
        err[0] = data[0];
        err[1] = data[1];
        err[2] = '\0';
        e = (char)atoi(err);
    }

    return e;

}

void enjoythesilence(void) {

    char ch, r;
    char outer = TRUE, inner = TRUE;

    POKE(JCL, 0);

    do {

        do {

            r = PEEK(668);

            cbm_k_chkin(1);
            ch = cbm_k_getin();
            cbm_k_clrch();

            if (PEEK(668) == r) {
                inner = FALSE;
            } else {
                POKE(JCL, 0);
            }

        } while (inner == TRUE);

    } while (PEEK(JCL) < 60);

}

void clear(char *str) {
    while (*str) {
        *str++ = '\0';
    }
}

void pad(char *str) {
    while(*str) {
        *str++ = PAD;
    }
}

void clearbuffer(void) {

    char ch;

    do {
        cbm_k_chkin(1);
        ch = cbm_k_getin();
        cbm_k_clrch();
    } while (PEEK(667) != PEEK(668));


}

void outbyte(char ch) {
    cbm_k_ckout(1);
    cbm_k_bsout(ch);
    cbm_k_clrch();
}

char inbyte(char delay, char *status) {

    char ch = 0, s, lch;
    char listening = TRUE;

    *status = OK;

    if (delay == 'S') {
        /* short delay ~ 10 secs */
        delay = 3;
    } else if (delay == 'L') {
        /* long delay ~ 100 secs */
        delay = 24;
    } else {
        delay = 3;
    }

    /* set jiffy clock low + med bytes to zero */
    POKE(JCM, 0);
    POKE(JCL, 0);

    do {

        s = PEEK(668);

        /* XLOCAL */
        lch = cbm_k_getin();
        if (lch != 0) {
            *status = CANCEL;
            ch = CAN;
            break;
        }

        /* XREMOTE */
        cbm_k_chkin(1);
        ch = cbm_k_getin();
        cbm_k_clrch();

        if (PEEK(668) != s) {
            listening = FALSE;
        }

        if (PEEK(JCM) > delay) {
            *status = TIMEOUT;
            print("?");
            listening = FALSE;
        }

    } while (listening == TRUE);

    return ch;

}
