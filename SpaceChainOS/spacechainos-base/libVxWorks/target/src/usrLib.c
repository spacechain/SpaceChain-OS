/**
 * @file
 * user library functions.
 *
 * VxWorks compatibility layer in SylixOS.
 *
 * Copyright (c) 2001-2014 SylixOS Group.
 * All rights reserved.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <api/Lw_Api_Kernel.h>
#include <api/Lw_Api_System.h>
#include "vxWorks.h"
#include "version.h"

/*
 * show pending thread
 */
void w (int taskNameOrId)
{
    API_ThreadPendShowEx(getpid());
}

void tw (int taskNameOrId)
{
    API_ThreadPendShowEx(getpid());
}

/*
 * stack check
 */
void checkStack (int taskNameOrId)
{
    API_StackShow();
}

/*
 * show task
 */
void i (int taskNameOrId)
{
    API_ThreadShowEx(getpid());
}

void ti (int taskNameOrId)
{
    API_ThreadShowEx(getpid());
}

/*
 * print vxworks logo
 */
void printLogo (void)
{
    static char logo[] =
    "\n"
    " ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]\n"
    " ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]\n"
    " ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]\n"
    "      ]]]]]]]]]]]  ]]]]     ]]]]]]]]]]       ]]              ]]]]         (R)\n"
    " ]     ]]]]]]]]]  ]]]]]]     ]]]]]]]]       ]]               ]]]]            \n"
    " ]]     ]]]]]]]  ]]]]]]]]     ]]]]]] ]     ]]                ]]]]            \n"
    " ]]]     ]]]]] ]    ]]]  ]     ]]]] ]]]   ]]]]]]]]]  ]]]] ]] ]]]]  ]]   ]]]]]\n"
    " ]]]]     ]]]  ]]    ]  ]]]     ]] ]]]]] ]]]]]]   ]] ]]]]]]] ]]]] ]]   ]]]]  \n"
    " ]]]]]     ]  ]]]]     ]]]]]      ]]]]]]]] ]]]]   ]] ]]]]    ]]]]]]]    ]]]] \n"
    " ]]]]]]      ]]]]]     ]]]]]]    ]  ]]]]]  ]]]]   ]] ]]]]    ]]]]]]]]    ]]]]\n"
    " ]]]]]]]    ]]]]]  ]    ]]]]]]  ]    ]]]   ]]]]   ]] ]]]]    ]]]] ]]]]    ]]]]\n"
    " ]]]]]]]]  ]]]]]  ]]]    ]]]]]]]      ]     ]]]]]]]  ]]]]    ]]]]  ]]]] ]]]]]\n"
    " ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]\n"
    " ]]]]]]]]]]]]]]]]]]]]]]]]]]]]]       Development System\n"
    " ]]]]]]]]]]]]]]]]]]]]]]]]]]]]";

    printf("%s\n", logo);
    printf("%s%s %s\n",
           " ]]]]]]]]]]]]]]]]]]]]]]]]]]]       ", runtimeName, runtimeVersion);
    printf("%s%s\n",
           " ]]]]]]]]]]]]]]]]]]]]]]]]]]       KERNEL: ", LIB_VXWORKS_VERSION);
    printf("%s\n",
           " ]]]]]]]]]]]]]]]]]]]]]]]]]       Copyright SylixOS Group. 2006-2017");
    printf("\n");

    API_SystemHwInfoPrint();

    printf("\n");
}

/*
 * print errno
 */
void printErrno (int errNo   /* status code whose name is to be printed */)
{
    printf("errno : %d [%s]\n", errNo, lib_strerror(errNo));
}

/*
 * end
 */
