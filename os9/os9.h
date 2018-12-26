typedef unsigned char u_char;

typedef struct os9_module_t {
  u_char id[2];
  u_char size[2];
  u_char name[2];
  u_char tyla;
  u_char atrv;
  u_char parity;
  union {
    u_char data[1];		/* plain modules */
    struct {
      u_char exec[2];
      u_char data[1];
    } system;
    struct {
      u_char exec[2];
      u_char mem[2];
      u_char data[1];
    } program;
    struct {
      u_char exec[2];
      u_char mem[2];
      u_char mode[1];
      u_char data[1];
    } driver;
    struct {
      u_char exec[2];
      u_char data[1];
    } file_mgr;
    struct {
      u_char fmgr[2];
      u_char driver[2];
      u_char mode;
      u_char port[3];
      u_char opt;
      u_char dtype;
      u_char data[1];
    } descriptor;
  } data;
} OS9_MODULE_t;

#define OS9_HEADER_SIZE 9

#define TYPE_MASK 0xF0
typedef enum os9_type_t {
  NULL_TYPE = 0,
  Prgrm, 
  Sbtrn, 
  Multi, 
  Data,  
  SSbtrn,
  TYPE_6,
  TYPE_7,
  TYPE_8,
  TYPE_9,
  TYPE_A,
  TYPE_B,
  Systm, 
  FlMgr,
  Drivr,
  Devic  
} OS9_TYPE_t;

#define LANG_MASK 0x0F
typedef enum os9_lang_t {
  NULL_LANG = 0,
  Objct,
  ICode,
  PCode,
  CCode,
  CblCode,
  FrtnCode,
  Obj6309,
} OS9_LANG_t;

#define ATTR_MASK 0xF0
typedef enum os9_attr_t {
  ReEnt   = 0x80,
  Modprot = 0x40,
} OS9_attr_t;

#define REVS_MASK 0x0F

#define OS9_ID0 0x87
#define OS9_ID1 0xcd

#define OS9_CRC0 0x80
#define OS9_CRC1 0x0F
#define OS9_CRC2 0xE3

#define INT(foo) (foo[0] * 256 + foo[1])
int os9_crc(OS9_MODULE_t *mod);
int os9_header(OS9_MODULE_t *mod);
