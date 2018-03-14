/**
 * @file
 * SylixOS SNMPv3 functions.
 */

/*
 * Copyright (c) 2016 Elias Oenal.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Author: Elias Oenal <lwip@eliasoenal.com>
 *         Dirk Ziegelmeier <dirk@ziegelmeier.net>
 *         Xu Guizhou <xuguizhou@acoinfo.com>
 */

#include <string.h>
#include <stdlib.h>
#include "lwip/snmp/snmpv3.h"
#include "lwip/err.h"
#include "lwip/def.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"

#if LWIP_SNMP && LWIP_SNMP_V3

#include "snmpv3_sylixos.h"

struct user_table_entry {
  char               username[32];
  snmpv3_auth_algo_t auth_algo;
  u8_t               auth_key[20];
  snmpv3_priv_algo_t priv_algo;
  u8_t               priv_key[20];
};

static struct user_table_entry user_table[SNMPV3_USER_MAX] = {
  { "sylixos", SNMP_V3_AUTH_ALGO_INVAL, "" , SNMP_V3_PRIV_ALGO_INVAL, "" },
};

static char snmpv3_engineid[32] = { 0 };
static u8_t snmpv3_engineid_len = 0;

static u32_t enginetime = 0;
static u32_t engineboots = 0;

#if SNMPV3_FILE_CONFIG > 0
static u8_t snmpv3_engineid_header[ENGINEID_HEADER] = { 0 };
static sys_mutex_t info_lock;
static sys_mutex_t user_lock;

#define INFO_LOCK() sys_mutex_lock(&info_lock)
#define USER_LOCK() sys_mutex_lock(&user_lock)

#define INFO_UNLOCK() sys_mutex_unlock(&info_lock)
#define USER_UNLOCK() sys_mutex_unlock(&user_lock)
#endif /* SNMPV3_FILE_CONFIG > 0 */

u32_t snmpv3_save_engine_boots_config(u32_t boots);
u32_t snmpv3_reset_engine_id_config(void);

/**
 * @brief   Get the user table entry for the given username.
 *
 * @param[in] username  pointer to the username
 *
 * @return              pointer to the user table entry or NULL if not found.
 */
static struct user_table_entry*
get_user(const char *username)
{
  size_t i;

  for (i = 0; i < LWIP_ARRAYSIZE(user_table); i++) {
    if (strnlen(username, 32) != strnlen(user_table[i].username, 32)) {
      continue;
    }

    if (memcmp(username, user_table[i].username, strnlen(username, 32)) == 0) {
      return &user_table[i];
    }
  }

  return NULL;
}

u8_t
snmpv3_get_amount_of_users(void)
{
  int i;
  struct user_table_entry *p;

  for (i = 0, p = user_table; i < LWIP_ARRAYSIZE(user_table); i++, p++) {
    if (!strcmp(p->username, "")) {
      break;
    }
  }

  return i;
}

/**
 * @brief Get the username of a user number (index)
 * @param username is a pointer to a string.
 * @param index is the user index.
 * @return ERR_OK if user is found, ERR_VAL is user is not found.
 */
err_t
snmpv3_get_username(char *username, u8_t index)
{
  if (index < LWIP_ARRAYSIZE(user_table)) {
    MEMCPY(username, user_table[index].username, sizeof(user_table[0].username));
    return ERR_OK;
  }

  return ERR_VAL;
}

/**
 * Timer callback function that increments enginetime and reschedules itself.
 *
 * @param arg unused argument
 */
static void
snmpv3_enginetime_timer(void *arg)
{
  LWIP_UNUSED_ARG(arg);
  
  enginetime++;

  /* This handles the engine time reset */
  snmpv3_get_engine_time_internal();

  /* restart timer */
  sys_timeout(1000, snmpv3_enginetime_timer, NULL);
}

err_t
snmpv3_set_user_auth_algo(const char *username, snmpv3_auth_algo_t algo)
{
  struct user_table_entry *p = get_user(username);

  if (p) {
    switch (algo) {
    case SNMP_V3_AUTH_ALGO_INVAL:
      if (p->priv_algo != SNMP_V3_PRIV_ALGO_INVAL) {
        /* Privacy MUST be disabled before configuring authentication */
        break;
      } else {
        p->auth_algo = algo;
        return ERR_OK;
      }
#if LWIP_SNMP_V3_CRYPTO
    case SNMP_V3_AUTH_ALGO_MD5:
    case SNMP_V3_AUTH_ALGO_SHA:
#endif
      p->auth_algo = algo;
      return ERR_OK;
    default:
      break;
    }
  }

  return ERR_VAL;
}

err_t
snmpv3_set_user_priv_algo(const char *username, snmpv3_priv_algo_t algo)
{
  struct user_table_entry *p = get_user(username);

  if (p) {
    switch (algo) {
#if LWIP_SNMP_V3_CRYPTO
    case SNMP_V3_PRIV_ALGO_AES:
    case SNMP_V3_PRIV_ALGO_DES:
      if (p->auth_algo == SNMP_V3_AUTH_ALGO_INVAL) {
        /* Authentication MUST be enabled before configuring privacy */
        break;
      } else {
        p->priv_algo = algo;
        return ERR_OK;
      }
#endif
    case SNMP_V3_PRIV_ALGO_INVAL:
      p->priv_algo = algo;
      return ERR_OK;
    default:
      break;
    }
  }

  return ERR_VAL;
}

err_t
snmpv3_set_user_auth_key(const char *username, const char *password)
{
  struct user_table_entry *p = get_user(username);
  const char *engineid;
  u8_t engineid_len;

  if (p) {
    /* password should be at least 8 characters long */
    if (strlen(password) >= 8) {
      memset(p->auth_key, 0, sizeof(p->auth_key));
      snmpv3_get_engine_id(&engineid, &engineid_len);
      switch (p->auth_algo) {
      case SNMP_V3_AUTH_ALGO_INVAL:
        return ERR_OK;
#if LWIP_SNMP_V3_CRYPTO
      case SNMP_V3_AUTH_ALGO_MD5:
        snmpv3_password_to_key_md5((const u8_t*)password, strlen(password), (const u8_t*)engineid, engineid_len, p->auth_key);
        return ERR_OK;
      case SNMP_V3_AUTH_ALGO_SHA:
        snmpv3_password_to_key_sha((const u8_t*)password, strlen(password), (const u8_t*)engineid, engineid_len, p->auth_key);
        return ERR_OK;
#endif
      default:
        return ERR_VAL;
      }
    }
  }

  return ERR_VAL;
}

err_t
snmpv3_set_user_priv_key(const char *username, const char *password)
{
  struct user_table_entry *p = get_user(username);
  const char *engineid;
  u8_t engineid_len;

  if (p) {
    /* password should be at least 8 characters long */
    if (strlen(password) >= 8) {
      memset(p->priv_key, 0, sizeof(p->priv_key));
      snmpv3_get_engine_id(&engineid, &engineid_len);
      switch (p->auth_algo) {
      case SNMP_V3_AUTH_ALGO_INVAL:
        return ERR_OK;
#if LWIP_SNMP_V3_CRYPTO
      case SNMP_V3_AUTH_ALGO_MD5:
        snmpv3_password_to_key_md5((const u8_t*)password, strlen(password), (const u8_t*)engineid, engineid_len, p->priv_key);
        return ERR_OK;
      case SNMP_V3_AUTH_ALGO_SHA:
        snmpv3_password_to_key_sha((const u8_t*)password, strlen(password), (const u8_t*)engineid, engineid_len, p->priv_key);
        return ERR_OK;
#endif
      default:
        return ERR_VAL;
      }
    }
  }

  return ERR_VAL;
}

/**
 * @brief   Get the storage type of the given username.
 *
 * @param[in] username  pointer to the username
 * @param[out] type     the storage type
 *
 * @return              ERR_OK if the user was found, ERR_VAL if not.
 */
err_t
snmpv3_get_user_storagetype(const char *username, snmpv3_user_storagetype_t *type)
{
  if (get_user(username) != NULL) {
    /* Found user in user table
     * In this dummy implementation, storage is permanent because no user can be deleted.
     * All changes to users are lost after a reboot.*/
    *type = SNMP_V3_USER_STORAGETYPE_PERMANENT;
    return ERR_OK;
  }

  return ERR_VAL;
}

/**
 *  @param username is a pointer to a string.
 * @param auth_algo is a pointer to u8_t. The implementation has to set this if user was found.
 * @param auth_key is a pointer to a pointer to a string. Implementation has to set this if user was found.
 * @param priv_algo is a pointer to u8_t. The implementation has to set this if user was found.
 * @param priv_key is a pointer to a pointer to a string. Implementation has to set this if user was found.
 */
err_t
snmpv3_get_user(const char* username, snmpv3_auth_algo_t *auth_algo, u8_t *auth_key, snmpv3_priv_algo_t *priv_algo, u8_t *priv_key)
{
  const struct user_table_entry *p;
  
  /* The msgUserName specifies the user (principal) on whose behalf the
     message is being exchanged. Note that a zero-length userName will
     not match any user, but it can be used for snmpEngineID discovery. */
  if(strlen(username) == 0) {
    return ERR_OK;
  }
  
  p = get_user(username);

  if (!p) {
    return ERR_VAL;
  }
  
  if (auth_algo != NULL) {
    *auth_algo = p->auth_algo;
  }

  if(auth_key != NULL) {
    MEMCPY(auth_key, p->auth_key, sizeof(p->auth_key));
  }

  if (priv_algo != NULL) {
    *priv_algo = p->priv_algo;
  }

  if(priv_key != NULL) {
    MEMCPY(priv_key, p->priv_key, sizeof(p->priv_key));
  }
  return ERR_OK;
}

/**
 * Get engine ID from persistence
 */
void
snmpv3_get_engine_id(const char **id, u8_t *len)
{
  *id = snmpv3_engineid;
  *len = snmpv3_engineid_len;
}

/**
 * Store engine ID in persistence
 */
err_t
snmpv3_set_engine_id(const char *id, u8_t len)
{
  if (len > sizeof(snmpv3_engineid)) {
    len = sizeof(snmpv3_engineid);
  }

  MEMCPY(snmpv3_engineid, id, len);
  snmpv3_engineid_len = len;

  return ERR_OK;
}

/**
 * Get engine boots from persistence. Must be increased on each boot.
 */
u32_t
snmpv3_get_engine_boots(void)
{
  return engineboots;
}

/**
 * Store engine boots in persistence
 */
void 
snmpv3_set_engine_boots(u32_t boots)
{
  engineboots = boots;

#if SNMPV3_FILE_CONFIG > 0
  snmpv3_save_engine_boots_config(boots);
#endif
}

/**
 * RFC3414 2.2.2.
 * Once the timer reaches 2147483647 it gets reset to zero and the
 * engine boot ups get incremented.
 */
u32_t
snmpv3_get_engine_time(void)
{
  return enginetime;
}

/**
 * Reset current engine time to 0
 */
void
snmpv3_reset_engine_time(void)
{
  enginetime = 0;
}

#if SNMPV3_FILE_CONFIG > 0

/**
 * Set snmpv3 user info by default value.
 */
u32_t
snmpv3_reset_user_table(void)
{
  FILE *fp;
  char buf[SNMPV3_BUF_MAX];
  struct user_table_entry *ptable;

  /* Clear user_table to zero */
  memset(user_table, 0, sizeof(user_table));

  USER_LOCK();

  fp = fopen(SNMPV3_USER, "w");
  if (fp == NULL) {
    USER_UNLOCK();
    return -1;
  }

  /* Write readme comments into file */
  sprintf(buf, "# config format as follow:\n" \
               "# user:authtype:authpassword:privtype:privpassword\n");
  fwrite(buf, strlen(buf), 1, fp);
  sprintf(buf, "# authtype: 0-noauth; 1-md5; 2-sha\n" \
               "# privtype: 0-nopriv; 1-des; 2-aes\n" \
               "# example: user:1:authpass:2:privpass\n");
  fwrite(buf, strlen(buf), 1, fp);

  /* Write default user into info file */
  sprintf(buf, "sylixos:1:sylixos_snmp:1:sylixos_snmp\n");
  fwrite(buf, strlen(buf), 1, fp);
  fclose(fp);

  ptable = &user_table[0];
  strcpy(ptable->username, "sylixos");
  snmpv3_set_user_auth_algo("sylixos", SNMP_V3_AUTH_ALGO_MD5);
  snmpv3_set_user_auth_key("sylixos", "sylixos_snmp");
  snmpv3_set_user_priv_algo("sylixos", SNMP_V3_PRIV_ALGO_DES);
  snmpv3_set_user_priv_key("sylixos", "sylixos_snmp");

  fclose(fp);

  USER_UNLOCK();
  return 0;
}

/**
 * Get snmpv3 user info from non-volatile memory
 */
void
parse_user_config(const char *buf, struct user_table_entry *user)
{
  char *p;
  char *split;
  char tmp[32];

  /**
   * Get user name from config
   */
  split = (char *)buf;
  p = strsep(&split, ":");
  if (split) {
    strncpy(user->username, p, strnlen(p, sizeof(user->username) - 1) + 1);
  }

  /**
   * Get auth algo from config
   * See snmpv3_auth_algo_t
   */
  if (split) {
    p = strsep(&split, ":");
    strncpy(tmp, p, strnlen(p, sizeof(tmp) - 1) + 1);
    sscanf(tmp, "%d", (int *)&user->auth_algo);
  }

  /**
   * Get auth password from config.
   * And convert to auth key
   */
  if (split) {
    p = strsep(&split, ":");
    /* Read auth password */
    strncpy(tmp, p, strnlen(p, sizeof(tmp) - 1) + 1);

    /* Convert auth password to auth key */
    snmpv3_set_user_auth_key(user->username, tmp);
  }

  /**
   * Get priv algo from config
   * See snmpv3_priv_algo_t
   */
  if (split) {
    p = strsep(&split, ":");
    strncpy(tmp, p, strnlen(p, sizeof(tmp) - 1) + 1);
    sscanf((const char *)tmp, "%d", (int *)&user->priv_algo);
  }

  /**
   * Get priv password from config.
   * And convert to priv key
   */
  if (split) {
    p = strsep(&split, ":");
    strncpy(tmp, p, strnlen(p, sizeof(tmp) - 1) + 1);
    sscanf(p, "%s\n", tmp);

    /* Convert priv password to priv key */
    snmpv3_set_user_priv_key(user->username, tmp);
  }
}

/**
 * Get snmpv3 user info from config file.
 */
void
snmpv3_get_user_table_config(void)
{
  FILE *fp;
  int  i;
  char *p;
  char buf[SNMPV3_BUF_MAX];
  struct user_table_entry *user;

  USER_LOCK();

  fp = fopen(SNMPV3_USER, "r");
  if (fp == NULL) {
    USER_UNLOCK();
    snmpv3_reset_user_table();
    return;
  }

  memset(buf, 0, sizeof(buf));
  memset(user_table, 0, sizeof(user_table));

  /**
   * Get line. var format is "sylixos:1:sylixos_snmp:1:sylixos_snmp"
   */
  for (i = 0, user = user_table; i < SNMPV3_USER_MAX; i++, user++) {
    if(fgets(buf, SNMPV3_BUF_MAX, fp) == NULL) {
      break;
    }

    /* Skip space & tab. */
    p = buf;
    while (*p == ' ' || (*p == '\t')) {
      p++;
    }

    /* Skip comments. */
    if (*p == '#') {
      continue;
    }
    parse_user_config(p, user);
  }

  fclose(fp);

  USER_UNLOCK();
}

/**
 * Save snmpv3 current value of engineid into file.
 */
u32_t
snmpv3_save_engine_boots_config(u32_t boots)
{
  FILE *fp;
  char buf[SNMPV3_BUF_MAX];

  INFO_LOCK();

  fp = fopen(SNMPV3_BOOTS, "w");
  if (fp == NULL) {
    INFO_UNLOCK();
    return -1;
  }
  memset(buf, 0, sizeof(buf));

  /* Write boots into file */
  sprintf(buf, "%d", boots);
  fwrite(buf, strlen(buf), 1, fp);
  fclose(fp);

  INFO_UNLOCK();
  return 0;
}

/**
 * Set snmpv3 engine info by default value.
 */
u32_t
snmpv3_reset_engine_boots(void)
{
  snmpv3_set_engine_boots(0);
  return (0);
}

/**
 * Get snmpv3 engine boots from non-volatile memory
 */
int
snmpv3_get_engine_boots_config(void)
{
  int fd;
  char buf[SNMPV3_BUF_MAX];

  INFO_LOCK();

  fd = open(SNMPV3_BOOTS, O_RDONLY);
  if (fd < 0) {
    INFO_UNLOCK();
    /* Reset default value in file */
    snmpv3_reset_engine_boots();
    return (-1);
  }
  memset(buf, 0, sizeof(buf));

  if (read(fd, buf, SNMPV3_BUF_MAX)) {
    /* Get boots from file. */
    sscanf(buf, "%d", &engineboots);
  }
  close(fd);

  INFO_UNLOCK();
  return (0);
}

/**
 * Set snmpv3 engine info by default value.
 */
u32_t
snmpv3_reset_engine_id_config(void)
{
  FILE *fp;
  char buf[32];

  memset(buf, 0, sizeof(buf));

  INFO_LOCK();

  fp = fopen(SNMPV3_ENGINEID, "w");
  if (fp == NULL) {
    INFO_UNLOCK();
    return (-1);
  }

  /* Prepare engineid header */
  memcpy(buf, snmpv3_engineid_header, sizeof(snmpv3_engineid_header));

  /* Copy mac data into engineid */
  strncpy(buf + ENGINEID_HEADER, ENGINEID_DEFAULT, 32 - ENGINEID_HEADER);

  /* Write engineid (without header) into file */
  fwrite(buf + ENGINEID_HEADER, strlen(ENGINEID_DEFAULT), 1, fp);
  fclose(fp);

  INFO_UNLOCK();

  snmpv3_set_engine_id(buf, strlen(ENGINEID_DEFAULT) + ENGINEID_HEADER);
  return (0);
}

/**
 * Set snmpv3 engine info by default value.
 */
u32_t
snmpv3_save_engine_id_config(void)
{
  FILE *fp;
  u8_t len;
  char *p;

  snmpv3_get_engine_id((const char **)&p, &len);
  /* Engine id is NOT set. Reset... */
  if (len == 0) {
    snmpv3_reset_engine_id_config();
    /* Should reset boots value if engine id change */
    snmpv3_reset_engine_boots();
    return 0;
  }

  INFO_LOCK();

  fp = fopen(SNMPV3_ENGINEID, "w");
  if (fp == NULL) {
    INFO_UNLOCK();
    return -1;
  }

  /* Write engineid (without header) into file. */
  p += 5;
  len -= 5;

  /* Write engineid into file */
  fwrite(p, len, 1, fp);
  fclose(fp);

  INFO_UNLOCK();
  return 0;
}

/**
 * Get snmpv3 engine info from non-volatile memory
 */
int
snmpv3_get_engine_id_config(void)
{
  int fd;
  size_t n;
  char buf[SNMPV3_BUF_MAX];

  INFO_LOCK();
  fd = open(SNMPV3_ENGINEID, O_RDONLY);
  if (fd < 0) {
    INFO_UNLOCK();
    /* Reset engine id */
    snmpv3_reset_engine_id_config();
    return  (-1);
  }

  /* Copy engineid header. TEXT type. */
  memcpy(buf, snmpv3_engineid_header, ENGINEID_HEADER);

  n = read(fd, buf + ENGINEID_HEADER, SNMPV3_BUF_MAX - ENGINEID_HEADER);
  close(fd);
  INFO_UNLOCK();

  if (n > 0) {
    snmpv3_set_engine_id(buf, n + ENGINEID_HEADER);
  }
  return (0);
}

/**
 * Initialize /etc/snmp & /var/log/snmp directory.
 */
static void
snmpv3_init_dir(void)
{
  if (access("/etc/snmp", F_OK) < 0) {
    mkdir("/etc/snmp", DEFAULT_DIR_PERM);
  }

  if (access("/var/log/snmp", F_OK) < 0) {
    mkdir("/var/log/snmp", DEFAULT_DIR_PERM);
  }
}

static void
snmpv3_init_engineid_header(void)
{
  int oid;
  u8_t *p = &snmpv3_engineid_header[0];

  oid = htonl(SNMP_LWIP_ENTERPRISE_OID);
  memcpy(p, &oid, sizeof(oid));
  p[4]  = ENGINEID_TYPE_TEXT;
  p[0] |= 0x80;
}
#endif /* SNMPV3_FILE_CONFIG > 0 */

/**
 * Initialize SylixOS SNMPv3 implementation
 */
void
snmpv3_sylixos_init(void)
{
#if SNMPV3_FILE_CONFIG > 0
  sys_mutex_new(&info_lock);
  sys_mutex_new(&user_lock);

  /* Check snmp director exists. */
  snmpv3_init_dir();

  /* Init snmp engineid header. */
  snmpv3_init_engineid_header();

  /* Get boots from config file. */
  if (snmpv3_get_engine_boots_config() == 0) {
    /* Increase boots if read config file successfully. */
    if (snmpv3_get_engine_boots() < SNMP_MAX_TIME_BOOT - 1) {
      snmpv3_set_engine_boots(snmpv3_get_engine_boots() + 1);
    } else {
      snmpv3_set_engine_boots(SNMP_MAX_TIME_BOOT);
    }
  }

  /**
   *  Get engine id from config file.
   *  Should get boots config before engine id.
   *  If get engine id fail, will reset boots config.
   */
  if (snmpv3_get_engine_id_config() != 0) {
    /* Should reset boots value if engine id change */
    snmpv3_reset_engine_boots();
  }

  /* Get user info from file */
  snmpv3_get_user_table_config();
#else
  snmpv3_set_engine_id("sylixos_engine", strlen("sylixos_engine"));
  snmpv3_set_user_auth_algo("sylixos", SNMP_V3_AUTH_ALGO_MD5);
  snmpv3_set_user_auth_key("sylixos", "sylixos_snmp");
  snmpv3_set_user_priv_algo("sylixos", SNMP_V3_PRIV_ALGO_DES);
  snmpv3_set_user_priv_key("sylixos", "sylixos_snmp");
#endif

  /* Start the engine time timer */
  snmpv3_enginetime_timer(NULL);
}

#endif /* LWIP_SNMP && LWIP_SNMP_V3 */
