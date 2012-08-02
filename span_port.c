/* span_port -- Mirror traffic ingress from the switch to a specific port or set of ports.

   Copyright (C) 2011 - beg0
   VDE-2 is Copyrighted 2005 by Renzo Davoli

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */
/*
TODO:
 - filter by ingress port (forward only some incoming port)
 - filter by ingress vlan

*/
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <config.h>
#include <vde.h>
#include <vdecommon.h>

#include <vdeplugin.h>

/*FIXME: this function is only defined if FSTP is defined in port.c (which is always the case as far as I understood)*/
extern void port_send_packet(int portno, void *packet, int len);

/* A linked-link structure that stores additional information about each port */
struct sp_cfg
{
    int portno;
    int span;    /* is it configured to mirror traffic */
    int active; /*is there someone connected to the port */
    struct sp_cfg* next;
};

/* The head of the linked-list */
struct sp_cfg* spc_head = NULL;


static int forward(struct dbgcl *tag,void *arg,va_list v);
static int setup_span_port(char *arg);
static int unsetup_span_port(char *arg);
static int print_ports(FILE *f);
static int port_creation(struct dbgcl *event,void *arg,va_list v);

/* Manage linked list of spc_head  */
static struct sp_cfg* find_span_port(int portno);
static struct sp_cfg* find_or_create_span_port(int portno);
static int del_span_port(int portno);

/* Description of the plugin */
struct plugin vde_plugin_data={
        .name="span_port",
        .help="Setup span port (aka mirroring port)",
};

/* CLI menu */
static struct comlist cl[]={
        {"span_port","============","Span port",NULL,NOARG},
        {"span_port/add","N","add a port to mirror traffic",setup_span_port,STRARG},
        {"span_port/del","N","delete a port to mirror traffic",unsetup_span_port,STRARG},
        {"span_port/printall","","print ports configured to do mirroring",print_ports,WITHFILE},
};

#define D_MIRROR 0101 
static struct dbgcl dl[]= {
         {"span_port/packetin","mirror incoming packet",D_MIRROR|D_IN},
         {"span_port/packetout","mirror outgoing packet",D_MIRROR|D_OUT},
};

/* Allocate and initialize a struct sp_cfg element, but do not enqueue it. */
static struct sp_cfg *new_sp_cfg(int portno)
{
    struct sp_cfg *ret = (struct sp_cfg*)malloc(sizeof(struct sp_cfg));
   
    if(!ret) return NULL;

    ret->portno = portno;
    ret->active = 0;
    ret->span = 0;
    ret->next = NULL;

    return ret;
}

/* Get span port configuration for a particular port. Create a new one if it does not exists already. */
struct sp_cfg *find_or_create_span_port(int portno)
{
        struct sp_cfg *cur = NULL;
        struct sp_cfg *prev = NULL;
        struct sp_cfg *tmp = NULL;

        if(portno <= 0 ) return NULL;

        if(!spc_head)
        {
            spc_head = new_sp_cfg(portno);

            return spc_head;
        }
        else {
           for(cur = spc_head; cur && cur->portno <= portno; cur = cur->next)
           {
             prev = cur;
             /* do we found our port? */
             if(cur->portno == portno) return cur ;
           }

           /* if we left the loop it means we did not find the port, create it!*/
           tmp = new_sp_cfg(portno);

           if(!tmp) return NULL; /*memory error*/

           tmp->next = cur;

           if(prev) {
              prev->next = tmp;
           }
           else {
              spc_head = tmp;
           }

           return tmp;

        }

        return NULL;
}

/* Find a span port configuration using port number. Return NULL if not found */
struct sp_cfg *find_span_port(int portno)
{
        struct sp_cfg *cur = NULL;

        if(portno <= 0 ) return NULL;

        if(!spc_head)
        {
            return NULL;
        }
        else {
           for(cur = spc_head; cur && cur->portno <= portno; cur = cur->next)
           {
             /* do we found our port? */
             if(cur->portno == portno) return cur ;
           }

           return NULL;
        }

        return NULL;

}

/* Remove (and free) a span port config from the linked list 
  return 0 if ok, EBADSLT if port do not mirror, EINVAL if memory error */
static int del_span_port(int portno)
{
        struct sp_cfg *cur = NULL;
        struct sp_cfg *prev = NULL;
        struct sp_cfg *tmp = NULL;

        if(portno <= 0 ) return EINVAL;

        if(!spc_head) {
           return EBADSLT;
        }
        else {
           for(cur=spc_head; cur && cur->portno != portno; cur = cur->next)
           { prev=cur; /*I'm just seeking */ }

           /* port not found */
           if(!cur) {
               return EBADSLT;
           }

           tmp = cur->next;
           free(cur);
           if(prev) {
               prev->next = tmp;
           }
           else
           {
              spc_head = tmp;
           }

           return 0;

        }

        return 0;
}

/*mark a port to mirror traffic */
int setup_span_port(char* arg)
{
    int portno = atoi(arg);
    
    if(portno <= 0 ) return EINVAL;

    struct sp_cfg *p = find_or_create_span_port(portno);

    if(!p) return EBADSLT;
    
    p->span = 1;

    return 0;
}

/* mark a port to no more mirror traffic */
int unsetup_span_port(char* arg)
{
    int portno = atoi(arg);

    if(portno <= 0 ) return EINVAL;

    struct sp_cfg *p = find_span_port(portno);

    if(!p) return EBADSLT;

    p->span = 0;

    return 0;
}

/* print which ports are configured to mirrror traffic */
static int print_ports(FILE *f)
{
         struct sp_cfg *cur = NULL;

         int found = 0;

         fprintf(f,"span ports: ");
         for(cur = spc_head; cur && cur->next; cur = cur->next) {
                 if(cur->span) {
                       found = 1;
                       fprintf(f,"%d, ",cur->portno);
                 }
         }
         if(cur && cur->span) {
                 found = 1;
                 fprintf(f,"%d\n",cur->portno);
         }

         if(!found)
                 fprintf(f, "N/A\n");

         return 0;
}

/* do the actual mirroring: look fo incoming packets and forward them to span ports */
static int forward(struct dbgcl *event,void *arg,va_list v)
{
        struct dbgcl *this=arg;
        struct sp_cfg *cur = NULL;

        switch (event->tag) {
/*                case D_PACKET|D_OUT:
                        this++;
*/
                case D_PACKET|D_IN:
                        {
                                int incoming_port=va_arg(v,int);

                                unsigned char *buf=va_arg(v,unsigned char *);
                                int len=va_arg(v,int);
                                for(cur = spc_head; cur; cur = cur->next) {
                                    if( cur->active && cur->span && (incoming_port != cur->portno))
                                    {
                                        void* dup = malloc(len+1);
                                        memcpy(dup, buf, len);
                                        port_send_packet(cur->portno, dup,len);
                                        free(dup);
                                    }
                                }
                        }
        }

        return 0;
}

/* monitor port creation/deletion */
static int port_creation(struct dbgcl *event,void *arg,va_list v) {
        struct dbgcl *this=arg;
        struct sp_cfg *spc = NULL;
        int portno;

        switch (event->tag) {
                case D_PORT|D_OUT:
                        portno = va_arg(v,int);
                        spc = find_or_create_span_port(portno);
                        if(spc) {
                                spc->active=0;
                        }
                        break;
                case D_PORT|D_IN:
                        portno = va_arg(v,int);
                        spc = find_or_create_span_port(portno);
                        if(spc) {
                                spc->active=1;
                        }
                        break;
        }

}


/* plugin load/unload functions */
	static void
	__attribute__ ((constructor))
init (void)
{
        int rv;

	ADDCL(cl);
	ADDDBGCL(dl);

        rv = eventadd(port_creation,"port",dl);
        rv = eventadd(forward,"packet",dl);
}

	static void
	__attribute__ ((destructor))
fini (void)
{
        struct sp_cfg *cur = spc_head;
        struct sp_cfg *tmp;
        int rv;

        /*free memory for linked list spc_head*/
        spc_head = NULL;
        while(cur) {
                tmp = cur->next;
                free(cur);
                cur = tmp;
        }

        rv = eventdel(port_creation,"port",dl);
        rv = eventdel(forward,"packet",dl);

	DELCL(cl);
	DELDBGCL(dl);
}
