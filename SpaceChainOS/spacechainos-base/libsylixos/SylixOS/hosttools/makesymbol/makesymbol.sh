#!/usr/bin/env bash

srcfile=libsylixos.a
symbolc=symbol.c
symbolh=symbol.h
NM=nm
funcfile=func.lst
objsfile=objs.lst

echo "create $symbolc from: $srcfile"

rm -f $funcfile $objfile 1>/dev/null 2>&1

for i in $srcfile; do 
    # function
    $NM $i | sed -n 's/.*\ [TW]\ \(.*\)/\1/gp' >>$funcfile;
    # obj, remove __sylixos_version
    $NM $i | sed -n 's/.*\ [BDRSCVG]\ \(.*\)/\1/gp' | sed '/__sylixos_version/d' >>$objsfile;
done

cat << EOF >$symbolc
/*********************************************************************************************************
 **                                                     
 **                                    中国软件开源组织                             
 **                                                        
 **                                   嵌入式实时操作系统                            
 **                                                        
 **                                       SylixOS(TM)                            
 **                                                        
 **                               Copyright  All Rights Reserved                        
 **                                                        
 **--------------文件信息-------------------------------------------------------------------------------- 
 **                                                        
 ** 文   件   名: `basename $symbolc`                     
 **                                                        
 ** 创   建   人: makesymbol.sh 工具                                        
 **                                                        
 ** 文件创建日期: `date`
 **                                                        
 ** 描        述: 系统 sylixos 符号表. (此文件由 makesymbol.sh 工具自动生成, 请勿修改)                
********************************************************************************************************/
#ifdef __GNUC__
#pragma GCC diagnostic warning "-w"
#endif

#include "symboltools.h"

#define SYMBOL_TABLE_BEGIN LW_STATIC_SYMBOL   _G_symLibSylixOS[] = {
                                                         
#define SYMBOL_TABLE_END };                                        
                                                       
#define SYMBOL_ITEM_FUNC(pcName)                \                                
    {   {(void *)0, (void *)0},                 \                                
        #pcName, (char *)pcName,                \                                
        LW_SYMBOL_TEXT                          \                                
    },                                                    
                                                        
#define SYMBOL_ITEM_OBJ(pcName)                 \                                
    {   {(void *)0, (void *)0},                 \                                
        #pcName, (char *)&pcName,               \                                
        LW_SYMBOL_DATA                          \                                
    },                                                    
                                                        
/*********************************************************************************************************
  全局对象声明                                                
*********************************************************************************************************/
EOF

# 声明
# extern int xxxx();
sed 's/\(.*\)/extern int \1\(\)\;/g' $funcfile >>$symbolc

cat << EOF >>$symbolc

/* objs */
EOF

# obj
# extern int xxxx;
sed 's/\(.*\)/extern int \1\;/g' $objsfile >>$symbolc


cat << EOF >>$symbolc

/*********************************************************************************************************
  系统静态符号表                                                
*********************************************************************************************************/
SYMBOL_TABLE_BEGIN                                                
EOF

sed 's/\(.*\)/    SYMBOL_ITEM_FUNC\(\1\)/g' $funcfile   >>$symbolc;
sed 's/\(.*\)/    SYMBOL_ITEM_OBJ\(\1\)/g' $objsfile   >>$symbolc;
cat $funcfile >tempfilesymbol.txt
cat $objsfile >>tempfilesymbol.txt

cat << EOF >>$symbolc
SYMBOL_TABLE_END                                                
/*********************************************************************************************************
  END                                                    
*********************************************************************************************************/
EOF

cat << EOF > $symbolh
/*********************************************************************************************************
 **                                                        
 **                                    中国软件开源组织                            
 **                                                        
 **                                   嵌入式实时操作系统                            
 **                                                        
 **                                       SylixOS(TM)                            
 **                                                        
 **                               Copyright  All Rights Reserved                        
 **                                                        
 **--------------文件信息--------------------------------------------------------------------------------
 **                                                        
 ** 文   件   名: `basename $symbolh`
 **                                                        
 ** 创   建   人: makesymbol.sh 工具                                        
 **                                                        
 ** 文件创建日期: `date`
 **                                                        
 ** 描        述: 系统 sylixos 符号表. (此文件由 makesymbol.sh 工具自动生成, 请勿修改)                
*********************************************************************************************************/
                                                    
#ifndef __SYMBOL_H                                                
#define __SYMBOL_H                                                
                                                        
#include "SylixOS.h"                                            
#include "symboltools.h"
                                                        
#define SYM_TABLE_SIZE      `cat tempfilesymbol.txt|wc -l`
extern  LW_STATIC_SYMBOL     _G_symLibSylixOS[SYM_TABLE_SIZE];
                                                        
static LW_INLINE  INT symbolAddAll (VOID)
{                                                        
    return  (symbolAddStatic((LW_SYMBOL *)_G_symLibSylixOS, SYM_TABLE_SIZE));                
}                                                        
#endif                                                                  /*  __SYMBOL_H                  */

/********************************************************************************************************* 
  END 
*********************************************************************************************************/
EOF

rm -f $funcfile $objsfile tempfilesymbol.txt

exit 0
