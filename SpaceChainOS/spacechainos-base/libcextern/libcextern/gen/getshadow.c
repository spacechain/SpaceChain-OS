/*
 *  POSIX 1003.1b - 9.2.2 - User Database Access Routines
 *
 *  SylixOS add this file
 *
 *  $Id: getshadow.c,v 1.13 2009/09/15 04:18:40 ralf Exp $
 */
 
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
static void init_etc_shadow (void)
{
  FILE *fp;
  static char etc_shadow_initted = 0;

  if (etc_shadow_initted)
    return;
  etc_shadow_initted = 1;
  
  /*
   *  Initialize /etc/shadow
   *  root 默认密码为 root
   *  sylixos 默认密码为 hanhui
   *  apps 默认密码为 apps
   *  hanhui 默认密码为 (暂时保密)
   */
  if (access("/etc/shadow", R_OK) < 0) {
    if ((fp = fopen("/etc/shadow", "w")) != NULL) {
      fprintf( fp, "root:$1$qY9g/6K4$/FKP3w1BsziKGCP3uLDnG.:0:0:99999:7:::\n"
                   "sylixos:$1$qY9g/6K4$WFEx17sxu/3aL3wE.u8NZ1:0:0:99999:7:::\n"
                   "apps:$1$qY9g/6K4$buV57yqE0kMbApOVI/jKM1:0:0:99999:7:::\n"
                   "hanhui:$1$qY9g/6K4$M/MZBr3WwSdMzIoXbVQGA.:0:0:99999:7:::\n"
                   "anonymous:!!:0:0:99999:7:::\n");
      fclose(fp);
      chmod("/etc/shadow", S_IRUSR);
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
  if (shadow_fp != NULL)
    fclose(shadow_fp);
}

/*
 * Extract a single shadow record from the database
 */
static int scansp (
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
    
      crypt_safe(pass, spwd.sp_pwdp, cCryptBuf, sizeof(cCryptBuf));
      if (!strcmp(cCryptBuf, spwd.sp_pwdp)) {
        ret = 0;
        break;
      }
    }
  }
  
  fclose(shadow);
  return ret;
}

/* end */

