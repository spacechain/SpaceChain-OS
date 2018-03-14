/*
 *  POSIX 1003.1b - 9.2.2 - User Database Access Routines
 *
 *  SylixOS add this file
 *
 *  $Id: getshadow.c,v 1.13 2009/09/15 04:18:40 ralf Exp $
 */
 
#define __SYLIXOS_KERNEL
#include "stdio.h"
#include "sys/types.h"
#include "pwd.h"
#include "grp.h"
#include "shadow.h"
#include "errno.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "limits.h"
#include "ctype.h"
#include "crypt.h"

int
scanString(FILE *fp, char **name, char **bufp, size_t *nleft, int nlFlag);
int
scanInt(FILE *fp, int *val);

static FILE *shadow_fp = NULL;
static char spbuf[256];
static struct spwd spwdent;

/*
 * Initialize useable but dummy databases
 */
void init_etc_shadow (void)
{
  FILE *fp;
  static char etc_shadow_initted = 0;

  if (etc_shadow_initted)
    return;
  etc_shadow_initted = 1;
  
  /*
   *  Initialize /etc/shadow
   *  root 默认密码为 root
   *  hanhui 默认密码为 (暂时保密)
   */
  if (access("/etc/shadow", R_OK) < 0) {
    if ((fp = fopen("/etc/shadow", "w")) != NULL) {
#if LW_CFG_SHELL_PASS_CRYPT_EN > 0
      fprintf(fp, "root:$1$qY9g/6K4$/FKP3w1BsziKGCP3uLDnG.:0:0:99999:7:::\n"
                  "hanhui:$1$U9mh7KP1$QriXGt1yOreNCt6voh9jT1:0:0:99999:7:::\n"
                  "anonymous:!!:0:0:99999:7:::\n");
#else
      fprintf(fp, "root:root:0:0:99999:7:::\n"
                  "hanhui:hanhui:0:0:99999:7:::\n"
                  "anonymous:!!:0:0:99999:7:::\n");
#endif /* LW_CFG_SHELL_PASS_CRYPT_EN > 0 */
      fclose(fp);
    }
  }
}

void setspent(void)
{
  init_etc_shadow();

  if (shadow_fp != NULL)
    fclose(shadow_fp);
  shadow_fp = fopen("/etc/shadow", "r");
}

void endspent(void)
{
  if (shadow_fp != NULL) {
    fclose(shadow_fp);
    shadow_fp = NULL;
  }
}

/*
 * Extract a single shadow record from the database
 */
int scansp (
  FILE *fp,
  struct spwd *spwd,
  char *buffer,
  size_t bufsize
)
{
  int sp_min = 0, sp_max = 0;
  int sp_warn = 0, sp_inact = 0;
  int sp_expire = 0, sp_flag = 0;
  int res;

  if (!scanString(fp, &spwd->sp_namp, &buffer, &bufsize, 0)
   || !scanString(fp, &spwd->sp_pwdp, &buffer, &bufsize, 0))
   return 0;
   
  scanInt(fp, &sp_min);
  scanInt(fp, &sp_max);
  scanInt(fp, &sp_warn);
  scanInt(fp, &sp_inact);
  scanInt(fp, &sp_min);
  scanInt(fp, &sp_flag);
  scanInt(fp, &res);
    
  spwd->sp_min    = sp_min;
  spwd->sp_max    = sp_max;
  spwd->sp_warn   = sp_warn;
  spwd->sp_inact  = sp_inact;
  spwd->sp_expire = sp_expire;
  spwd->sp_flag   = sp_flag;
  return 1;
}

int getspent_r(struct spwd *result_buf, char *buffer,
		       size_t buflen, struct spwd **result)
{
    init_etc_shadow();
    
    if ((shadow_fp == NULL) || (result_buf == NULL)) {
      errno = EINVAL;
      return -1;
    }
    
    if (!scansp(shadow_fp, result_buf, buffer, buflen)) {
      errno = EINVAL;
      return -1;
    }
    
    *result = result_buf;
    
    return  0;
}

int getspnam_r(const char *name, struct spwd *result_buf,
		       char *buffer, size_t buflen,
		       struct spwd **result)
{
    init_etc_shadow();
    
    if ((shadow_fp == NULL) || 
        (result_buf == NULL) ||
        (name == NULL)) {
      errno = EINVAL;
      return -1;
    }
    
    for (;;) {
      if (!scansp(shadow_fp, result_buf, buffer, buflen)) {
        errno = EINVAL;
        return -1;
      }
      if (strcmp(result_buf->sp_namp, name) == 0) {
        *result = result_buf;
        return 0;
      }
    }
    
    errno = EINVAL;
    return -1;
}

int fgetspent_r(FILE *stream, struct spwd *result_buf,
			    char *buffer, size_t buflen,
			    struct spwd **result)
{
    init_etc_shadow();
    
    if ((stream == NULL) || 
        (result_buf == NULL)) {
      errno = EINVAL;
      return -1;
    }
    
    if (!scansp(stream, result_buf, buffer, buflen)) {
      errno = EINVAL;
      return -1;
    }
    
    *result = result_buf;
    
    return  0;
}

struct spwd *getspent(void)
{
  struct spwd *p;

  if (getspent_r(&spwdent, spbuf, sizeof(spbuf), &p)) {
    return NULL;
  }
  return p;
}

struct spwd *getspnam(const char *name)
{
  struct spwd *p;

  if (getspnam_r(name, &spwdent, spbuf, sizeof(spbuf), &p)) {
    return NULL;
  }
  return p;
}

struct spwd *fgetspent(FILE *stream)
{
  struct spwd *p;

  if (fgetspent_r(stream, &spwdent, spbuf, sizeof(spbuf), &p)) {
    return NULL;
  }
  return p;
}

int putspent(struct spwd *p, FILE *stream)
{
  if (!p || !stream) {
    errno = EINVAL;
    return -1;
  }

  if (fprintf(stream, "%s:%s:%ld:%d:%d:%d:%d:%d:%d:\n", 
              p->sp_namp, 
              p->sp_pwdp, 
              p->sp_lstchg,
              p->sp_min,
              p->sp_max,
              p->sp_warn,
              p->sp_inact,
              p->sp_expire,
              p->sp_flag) > 0) {
    return 0;
  } else {
    return -1;
  }
}
/*
 * User Password Check
 */
int passwdcheck (const char *name, const char *pass)
{
  struct spwd    spwd;
  struct passwd  user;
  struct passwd  *puser;
  char  buf[256];
  FILE *shadow;
  int ret = -1;
  
  init_etc_shadow();

  if (getpwnam_r(name, &user, buf, sizeof buf, &puser)) {
    return ret;
  }
  
  if (strcmp(puser->pw_passwd, "x")) { /* not in shadow */
    return ret;
  }
  
  shadow = fopen("/etc/shadow", "r");
  if (shadow == NULL) {
    return ret;
  }
  
  for (;;) {
    if (!scansp(shadow, &spwd, buf, sizeof buf)) {
      break;
    }
    if (!strcmp(name, spwd.sp_namp)) {
      char  cCryptBuf[128];
      
      if (!strcmp("!!", spwd.sp_pwdp)) { /* no passwd */
        ret = 0;
        break;
      }
    
#if LW_CFG_SHELL_PASS_CRYPT_EN > 0
      crypt_safe(pass, spwd.sp_pwdp, cCryptBuf, sizeof(cCryptBuf));
#else
      strlcpy(cCryptBuf, pass, sizeof(cCryptBuf));
#endif /* LW_CFG_SHELL_PASS_CRYPT_EN > 0 */
      
      if (!strcmp(cCryptBuf, spwd.sp_pwdp)) {
        errno = EBADMSG;
        ret = 0;
        break;
      }
    }
  }
  
  fclose(shadow);
  return ret;
}

/*
 * login_defs
 */
struct login_defs {
  int pass_max_days;    /* default 90  */
  int pass_min_days;    /* default 0   */
  int pass_min_len;     /* default 8   */
  int pass_warn_age;    /* default 7   */
  int pass_delay_ms;    /* default 500 */
  int fail_delay;       /* default 2   */
  int faillog_enab;     /* default 0   */
  int syslog_su_enab;   /* default 0   */
  int syslog_sg_enab;   /* default 0   */
  int md5_crypt_enab;   /* default 1   */
};

static struct login_defs login_defines = {90, 0, 8, 7, 500, 2, 0, 0, 0, 1};
 
/*
 * get yes or no value
 */
static void str_get_yes_no (char *str, int *get)
{
  while (*str && (*str == ' ' || *str == '\t')) {
    str++;
  }
  
  if (*str == '\0') {
    return;
  }
  
  if (strncasecmp(str, "yes", 3) == 0) {
    *get = 1;
  } else if (strncasecmp(str, "no", 2) == 0) {
    *get = 0;
  }
}

/*
 * get int value
 */
static void str_get_int (char *str, int *get)
{
  while (*str && (*str == ' ' || *str == '\t')) {
    str++;
  }
  
  if (*str == '\0') {
    return;
  }
  
  *get = atoi(str);
}
/*
 * /etc/login.defs
 */
static void login_defs_init (void)
{
  static int isload = 0;
  char line_buf[PATH_MAX+1];
  FILE *logindefs;
  
  if (isload) {
    return;
  }
  
  logindefs = fopen("/etc/login.defs", "r");
  if (logindefs == NULL) {
    return;
  }
  
  while (fgets(line_buf, PATH_MAX+1, logindefs) != NULL) {
    char *value;
    
    if (line_buf[0] == '#') {
      continue;
    }
    
#define PASS_MAX_DAYS       "PASS_MAX_DAYS"
#define PASS_MAX_DAYS_LEN   13

#define PASS_MIN_DAYS       "PASS_MIN_DAYS"
#define PASS_MIN_DAYS_LEN   13

#define PASS_MIN_LEN        "PASS_MIN_LEN"
#define PASS_MIN_LEN_LEN    12

#define PASS_WARN_AGE       "PASS_WARN_AGE"
#define PASS_WARN_AGE_LEN   13

#define PASS_DELAY          "PASS_DELAY"
#define PASS_DELAY_LEN      10

#define PASS_DELAY_MS       "PASS_DELAY_MS"
#define PASS_DELAY_MS_LEN   13

#define FAIL_DELAY          "FAIL_DELAY"
#define FAIL_DELAY_LEN      10

#define FAILLOG_ENAB        "FAILLOG_ENAB"
#define FAILLOG_ENAB_LEN    12

#define SYSLOG_SU_ENAB      "SYSLOG_SU_ENAB"
#define SYSLOG_SU_ENAB_LEN  14

#define SYSLOG_SG_ENAB      "SYSLOG_SG_ENAB"
#define SYSLOG_SG_ENAB_LEN  14

#define MD5_CRYPT_ENAB      "MD5_CRYPT_ENAB"
#define MD5_CRYPT_ENAB_LEN  14
    
    if (strncmp(line_buf, PASS_MAX_DAYS, PASS_MAX_DAYS_LEN) == 0) {
      value = line_buf + PASS_MAX_DAYS_LEN;
      str_get_int(value, &login_defines.pass_max_days);
    
    } else if (strncmp(line_buf, PASS_MIN_DAYS, PASS_MIN_DAYS_LEN) == 0) {
      value = line_buf + PASS_MIN_DAYS_LEN;
      str_get_int(value, &login_defines.pass_min_days);
    
    } else if (strncmp(line_buf, PASS_MIN_LEN, PASS_MIN_LEN_LEN) == 0) {
      value = line_buf + PASS_MIN_LEN_LEN;
      str_get_int(value, &login_defines.pass_min_len);
    
    } else if (strncmp(line_buf, PASS_WARN_AGE, PASS_WARN_AGE_LEN) == 0) {
      value = line_buf + PASS_WARN_AGE_LEN;
      str_get_int(value, &login_defines.pass_warn_age);
    
    } else if (strncmp(line_buf, PASS_DELAY, PASS_DELAY_LEN) == 0) {
      value = line_buf + PASS_DELAY_LEN;
      str_get_int(value, &login_defines.pass_delay_ms);
      login_defines.pass_delay_ms *= 1000;
      
    } else if (strncmp(line_buf, PASS_DELAY_MS, PASS_DELAY_MS_LEN) == 0) {
      value = line_buf + PASS_DELAY_MS_LEN;
      str_get_int(value, &login_defines.pass_delay_ms);
      
    } else if (strncmp(line_buf, FAIL_DELAY, FAIL_DELAY_LEN) == 0) {
      value = line_buf + FAIL_DELAY_LEN;
      str_get_int(value, &login_defines.fail_delay);
    
    } else if (strncmp(line_buf, FAILLOG_ENAB, FAILLOG_ENAB_LEN) == 0) {
      value = line_buf + FAILLOG_ENAB_LEN;
      str_get_yes_no(value, &login_defines.faillog_enab);
      
    } else if (strncmp(line_buf, SYSLOG_SU_ENAB, SYSLOG_SU_ENAB_LEN) == 0) {
      value = line_buf + SYSLOG_SU_ENAB_LEN;
      str_get_yes_no(value, &login_defines.syslog_su_enab);
    
    } else if (strncmp(line_buf, SYSLOG_SG_ENAB, SYSLOG_SG_ENAB_LEN) == 0) {
      value = line_buf + SYSLOG_SG_ENAB_LEN;
      str_get_yes_no(value, &login_defines.syslog_sg_enab);
    
    } else if (strncmp(line_buf, MD5_CRYPT_ENAB, MD5_CRYPT_ENAB_LEN) == 0) {
      value = line_buf + MD5_CRYPT_ENAB_LEN;
      str_get_yes_no(value, &login_defines.md5_crypt_enab);
    }
  }
  
  fclose(logindefs);
  isload = 1;
}
 
/*
 * User Login
 */
int userlogin (const char *name, const char *pass, int pass_delay_en)
{
  int  chk;
  
  login_defs_init();
  
  chk = passwdcheck(name, pass);
  
  if (chk == 0) {
    struct passwd  passwd;
    struct passwd  *passwdRes;
    CHAR           cBuf[256];
    
    if (pass_delay_en && login_defines.pass_delay_ms) {
      API_TimeMSleep(login_defines.pass_delay_ms);  /* wait some mirco-seconds... */
    }
    
    if (getpwnam_r(name, &passwd, cBuf, sizeof(cBuf), &passwdRes) == 0) {
      API_ThreadTcbSelf()->TCB_euid = 0; /* force effective root */
      
      initgroups(name, passwd.pw_gid);
        
      setgid(passwd.pw_gid);    /* change id to current user */
      setuid(passwd.pw_uid);
      return 0;
    }
  } else {
    if (access("/etc/passwd", R_OK) && access("/etc/shadow", R_OK)) {
      _DebugHandle(__ERRORMESSAGE_LEVEL, "/etc/passwd & /etc/shadow files not found, "
                                         "password check override.\r\n");
      return 0; /* file system not init, passwd check none */
    }
    /* todo:need save syslog */
    sleep(login_defines.fail_delay);    /* wait fail_delay seconds */
  }
  
  return -1;
}

/*
 * Get pass
 */
char *getpass_r (const char *prompt, char *buffer, size_t buflen)
{
#if LW_CFG_DEVICE_EN > 0
  INT iOldOpt;
  ssize_t sstRet;
  char *pcRet = NULL;

  if (!buffer || !buflen) {
    errno = EINVAL;
    return NULL;
  }
  
  if (!isatty(STD_IN)) {
    errno = ENOTTY;
    return NULL;
  }
  
  ioctl(STD_IN, FIOGETOPTIONS, &iOldOpt);
  ioctl(STD_IN, FIOSETOPTIONS, (OPT_TERMINAL & ~(OPT_ECHO | OPT_MON_TRAP)));
  
  if (prompt) {
    write(STD_OUT, prompt, lib_strlen(prompt));
  }
  
  sstRet = read(STD_IN, buffer, buflen);
  if (sstRet <= 0) {
    goto __out;
  }
  
  buffer[sstRet - 1] = PX_EOS;
  pcRet = buffer;

__out:
  ioctl(STD_IN, FIOFLUSH);
  ioctl(STD_IN, FIOSETOPTIONS, iOldOpt);
  write(STD_OUT, "\n", 1);
  return pcRet;
  
#else
  errno = ENOSYS;
  return NULL;
#endif
}
 
char *getpass (const char *prompt)
{
  static char cPass[PASS_MAX + 1];
  
  return getpass_r(prompt, cPass, sizeof(cPass));
}

/* end */
