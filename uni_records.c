/*
* Com_Sheet(v0.4 beta) - uni_records.c
* Author :: Debjoy Das <debd92@gmail.com>
*           Copyright (C) 2013
* Released under           :: GPL v3
* Release date (v0.4 beta) :: 25-01-2013
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "formulae.h"

#define STR_LEN 100
#define MAX_FILE_LINE_LEN 200
#define MAX_FXS 50
#define UPDATE_MAX_LEN(a, b) strlen(a) > b ? strlen(a)+1 : b

struct record {
  struct record *link;
  char *rec_col_ptr[];
  };

struct col_lbl {
  struct col_lbl *link;
  int fx;
  int max_len;
  char l_name[];
  };

typedef struct record rec;
typedef struct col_lbl label;

enum er_codes { INVALID_POS, REC_NOT_FOUND, FIELD_NOT_FOUND, LBL_NOT_FOUND, REC_EMPT, WRITE_FAIL, FAILD_TO_OPEN_STRM, WRONG_COMM_ERR, ABORT_PARSE };

unsigned int max_label_len=0;
char *formulaes[MAX_FXS];
int fxs=0;

char *yellow = "\E[0;33m";
char *red = "\E[0;31m";
char *norm = "\E[0m\017";
char *bcyan = "\E[1;36m";

int hndl_fatal_err(char *msg);
void hndl_user_err(int ERR_CODE);
void getstring(char *name, int b_size, char if_update);
void getnums(int *nm);
void ins_row_beg(rec **list, label **lbl, int *lbl_count);
void ins_row_end(rec **list, rec *f_node, label **lbl, int *lbl_count);
void ins_row_any(rec **list, label **lbl, int *lbl_count, int pos);
int ins_col(rec **tlist, label **lbl, int *lbl_count, int pos, bool ins);
void ins_formulae_col(rec **tlist, label **lbl, int *lbl_count);
int expr_resolver(rec *tnode, label *t_lbl, char *expr_str, char *expr_resolved, int lbl_count, int expr_len);
void del_row(rec **list, char op, label *lbl, int lbl_count, int pos);
void del_col(rec **tlist, label **lbl, int *lbl_count, int pos);
void list_disp(rec *tlist, label *lbl, char mode);
void prepare_node(rec **tnode, label **lbl, int *lbl_count, int pos, bool ins, rec *entire_list, label *entire_lbl);
void srch_record(rec *tlist, label *lbl);
void sort_record(rec **tlist, label *lbl, int lbl_count);
void edit_field(rec *tlist, label *lbl, char *row, char *col, char mode);
char *get_line(char *looper, char *buff);
void write_record(rec *tlist, label *lbl, int lbl_count, char *f_name, char format);
void read_record(rec **tlist, label **lbl, int *lbl_count, char *f_name);
void freemem(rec *tlist, label *lbl, int lbl_count, char mode);

int hndl_fatal_err(char *msg)
{
  perror((const char *)msg);
  exit(EXIT_FAILURE);
}

void hndl_user_err(int ERR_CODE)
{
  fputs(red, stderr);
  switch(ERR_CODE)
  {
    case INVALID_POS:
         fputs("\nInvalid Position.\n\n", stderr);
         break;
    case FIELD_NOT_FOUND:
         fputs("\nGiven field doesn't exist.\n\n", stderr);
         break;
    case LBL_NOT_FOUND:
         fputs("\nGiven label doesn't exist.\n\n", stderr);
         break;
    case REC_EMPT:
         fputs("\nRecord is Empty.\n\n", stderr);
         break;
    case FAILD_TO_OPEN_STRM:
         perror("fopen");
         break;
    case WRITE_FAIL:
         perror("fprintf");
         break;
    case WRONG_COMM_ERR:
         fputs("\nCommand syntax error. See help.\n\n", stderr);
         break;
    case ABORT_PARSE:
         fputs("\nFile not in Com_Sheet parsable format. Aborting.\n\n", stderr);
         break;
  }
  fputs(norm, stderr);
  puts("Press Enter");
  getchar();
}

void show_help(void)
{
  puts("\nCommands::\n\
\n\
insert row at begining  :: irb\n\
       row at end       :: ire\n\
       row anywhere     :: ira <position>\n\
\n\
delete row at begining  :: drb\n\
       row at end       :: dre\n\
       row anywhere     :: dra <position>\n\
\n\
insert column           :: ica <position>\n\
insert formulae column  :: icf\n\
delete column           :: dca <position>\n\
\n\
edit field              :: edf <row identifier> <column name>\n\
edit label              :: edl <column name>\n\
search record           :: sch\n\
sort   record           :: srt\n\
\n\
write record (tabular)  :: wrt <filename>\n\
write record (parsable) :: wrp <filename>\n\
read record             :: rdf <filename>\n\
\n\
show help               :: help\n\
clear screen            :: cls\n\
quit                    :: q\n\
\n\
Note: position can be a neumerical value or\n\
'b' stating begining or\n\
'e' stating end\n\
Numeric positions start from 1.\n\
\n\
Row identifier of a row is the first element of that row.\n\
See FORMULAES for a note on formulaes.\n");

}

void getstring(char *name, int b_size, char if_update)
{
  int i=0;
  char ch;
  unsigned int ip_len;
  /* avoid \n from last scanf */
  /* if((ch = fgetc(stdin)) != '\n') name[i++] = ch; */
  while((ch = fgetc(stdin)) != '\n' && i < b_size-1)
   name[i++] = ch;
  name[i] = '\0';
  if(if_update == 'u' && ((ip_len = strlen(name)) > max_label_len))
    max_label_len = ip_len+1;

  /*scanf("%99[A-Za-z ]",name);*/
}

void getnums(int *nm)
{
  char str[10];
  unsigned short int i=0;
  
  getstring(str, 10, 'n');

  for(; str[i]; i++)
  {
    if(!isdigit(str[i])) {
      fputs("Not a number. Enter again: ", stdout);
      getstring(str, 10, 'n');
      i=0;
    }
  }
  *nm = atoi(str);
}

void prepare_node(rec **tnode, label **lbl, int *lbl_count, int pos, bool ins, rec *entire_list, label *entire_lbl)
{
  label *temp=*lbl;
  int col_indx=0;

  if(!(*lbl)) { ins_col(tnode, lbl, lbl_count, 0, 0); temp=*lbl; }
  puts("");
  /* allocate space for the char pointers to label data */
  if(!ins)
  {
    if(!(*tnode = malloc(sizeof(rec) + *lbl_count * sizeof(char *))))
      hndl_fatal_err("malloc");
  }
  else col_indx = pos;

  while(temp && col_indx < pos + *lbl_count)
  {
    short ser=1;
    char ip_buff[STR_LEN];
    char *temp_buff=NULL;
    if(!temp->fx)
    {
      if(ins)
      {
        if(pos && *tnode && temp == *lbl) printf("For \"%s%s%s\"\n", yellow,\
        (*tnode)->rec_col_ptr[0], norm);
        else
          if(temp == *lbl) printf("For row %d\n", ser);
      }
      printf("%-*s: ", max_label_len, temp->l_name);
      getstring(ip_buff, STR_LEN, 'n');
      if(!(temp_buff = malloc(sizeof(ip_buff) + 1)))
        hndl_fatal_err("malloc");
      strcpy(temp_buff, ip_buff);
      (*tnode)->rec_col_ptr[col_indx++] = temp_buff;
      temp->max_len = UPDATE_MAX_LEN((*tnode)->rec_col_ptr[col_indx-1], temp->max_len);
    }
    else
    {
      char *expr_str = formulaes[temp->fx - 1];
      unsigned int expr_len = strlen(expr_str);
      char expr_resolved[STR_LEN];
           bzero(expr_resolved, STR_LEN);
      expr_resolver(*tnode, entire_lbl, expr_str, expr_resolved, *lbl_count, expr_len);

      char postfx[2*expr_len+1]; /* length of the postfx buffer is kept such so to use space as delimiters */
           bzero(postfx, 2*expr_len+1);
      to_postfix(expr_resolved, postfx, 2*expr_len+1);

      float temp_res = evaluate(postfx, 2*expr_len+1);
      /*printf("%.2f\n", temp_res);
      sleep(1);*/
      char *to_char = malloc(STR_LEN/2 * sizeof(char));
      sprintf(to_char, "%g", temp_res);
      (*tnode)->rec_col_ptr[col_indx++] = to_char;
    }

    temp = temp->link;
  }
}

void ins_row_beg(rec **list, label **lbl, int *lbl_count)
{
  rec *newn=NULL;

  prepare_node(&newn, lbl, lbl_count, 0, 0, *list, *lbl);
  if(!*list) {
    *list = newn;
    newn->link = NULL;
  }
  else {
    newn->link = *list;
    *list = newn;
  }
}

void ins_row_end(rec **list, rec *f_node, label **lbl, int *lbl_count)
{
  rec *newn=NULL, *temp=*list;

  if(f_node) newn=f_node;
  else prepare_node(&newn, lbl, lbl_count, 0, 0, *list, *lbl);

  if(!*list) {
    *list = newn;
    newn->link = NULL;
  }
  else
  {
    while(temp->link)
      temp = temp->link;
    temp->link = newn;
    newn->link = NULL;
  }
}

void ins_row_any(rec **list, label **lbl, int *lbl_count, int pos)
{
  rec *newn=NULL, *temp=*list;
  int i=0;

  if(!temp)
  { 
    if(!pos) ins_row_beg(list, lbl, lbl_count);
    else hndl_user_err(INVALID_POS);
    return;
  }
  while(pos!=1 && i<pos)
  {
    if(!temp->link && i+1!=pos) { hndl_user_err(INVALID_POS); return; }
    if(temp->link) temp = temp->link;
    i++;
  }

  prepare_node(&newn, lbl, lbl_count, 0, 0, *list, *lbl);
  if(!pos) {
    newn->link = *list;
   *list = newn;
  } 
  else {
    newn->link = temp->link;
    temp->link = newn;
  }
}

int ins_col(rec **tlist, label **lbl, int *lbl_count, int pos, bool ins)
{
  int i=0, old_lbl_count=0;
  label *temp=NULL, *trav=*lbl, *keep_trav=NULL;
  char temp_buff[STR_LEN];
       bzero(temp_buff, STR_LEN);
  
  if(ins)
  {
    if(pos<0 || pos>*lbl_count) { hndl_user_err(INVALID_POS); return -1; }

    int indx=0;
    while(indx++ < pos-1)
      trav = trav->link; 
  }
  
  if(*lbl_count) { old_lbl_count = *lbl_count; keep_trav=trav; }
  fputs("Number of cols to add: ", stdout);
  getnums(lbl_count);
  puts("");
  
  int temp_pos=pos;
  for(; i<*lbl_count; i++)
  {
    printf("Name Column %d : ", i+pos+1);
    getstring(temp_buff, STR_LEN, 'u');

    /* allocate space for storing the label name*/
    if(!(temp = malloc(sizeof(label) + (int)strlen(temp_buff) + 2))) 
      hndl_fatal_err("malloc");
    temp->fx=0;
    temp->max_len=0;

    strcpy(temp->l_name, temp_buff);
    temp->max_len = UPDATE_MAX_LEN(temp_buff, temp->max_len);

    if(!trav) temp->link=NULL;
    else temp->link=trav->link;
    if(!(*lbl) || (ins && !temp_pos))
      { temp->link=*lbl; *lbl = temp; temp_pos=1; }
    else  trav->link = temp;
    trav = temp;
  }
  
  /* if list is empty, this handles the request far cleanly */
  if(!(*tlist) && ins)
  {
    prepare_node(tlist, lbl, lbl_count, 0, 0, NULL, NULL);
    return;
  }

  if(ins)
  {
    rec *keep=NULL, *prev_node=NULL;
    int offset = *lbl_count; /* number of new entries */
    *lbl_count += old_lbl_count;  /* total no. entries currently */
    while((*tlist))
    {
      /* allocate new space for storing ptrs to new labels */
      *tlist = realloc(*tlist, sizeof(rec) + (*lbl_count * sizeof(char *)));
      if(!(*tlist))
        hndl_fatal_err("malloc");

      if(prev_node) prev_node->link=*tlist;
      if(!keep) keep = *tlist;

      int i=*lbl_count-1;
      /* right shft the required label ptrs so to fit new labels in pos */
      while(i >= pos+offset)
      {
        (*tlist)->rec_col_ptr[i] = (*tlist)->rec_col_ptr[i - offset];
        (*tlist)->rec_col_ptr[i - offset] = NULL;
        i--;
      }
      prev_node = *tlist;
      (*tlist) = (*tlist)->link;
    }
    (*tlist) = keep; /* restore *tlist */
    fputs("\nInitialize new fields:\n", stdout);
    if(pos) keep_trav = keep_trav->link; /* we only want to initialize new labels */
    else keep_trav = *lbl;
    keep=(*tlist);
    while(keep)
    {
      /* corresponds:: offset = num of lbls to init, pos = from label pos */
      prepare_node(&keep, &keep_trav, &offset, pos, 1, NULL, NULL);
      keep=keep->link;
    }
  }

  return 0;
}

int expr_resolver(rec *tnode, label *t_lbl, char *expr_str, char *expr_resolved, int lbl_count, int expr_len)
{
  struct label_cache {  /* build a label data cache so we dont have           */
         char *name;    /* to go through the entire linked list every time    */ 
         char *data;
         } cache[lbl_count];
  int indx=0;
  int fx_indx=0;

  while(t_lbl) { cache[indx++].name = t_lbl->l_name; t_lbl = t_lbl->link; }

  bzero(expr_resolved, STR_LEN);
  for(indx=0; indx<lbl_count; indx++) 
    cache[indx].data = tnode->rec_col_ptr[indx];

  indx=0;
  while(indx <= expr_len)
  {
    short match_f=0;
    int t_indx=0; 
    char temp_name[STR_LEN];
         bzero(temp_name, STR_LEN);
    
    if(isalpha(expr_str[indx]))
    {
      t_indx=0;
      while(isalpha(expr_str[indx]))
        temp_name[t_indx++] = expr_str[indx++];
      temp_name[t_indx] = '\0'; 

      for(t_indx=0; t_indx<lbl_count; t_indx++)
      {
        if(!strcmp(temp_name, cache[t_indx].name))
        {
          strcat(expr_resolved, cache[t_indx].data);
          fx_indx = strlen(expr_resolved);
          match_f=1; break; 
        }
      }

      if(!match_f)
        return 1;
    }
    else
     expr_resolved[fx_indx++] = expr_str[indx++];
  }
  return 0;
}

void ins_formulae_col(rec **tlist, label **lbl, int *lbl_count)
{
  if(!*lbl || !*tlist) { hndl_user_err(REC_EMPT); return; }

  label *t_lbl=*lbl;
  rec *t_list=*tlist;
  int col_indx = *lbl_count;

  /* get new label name and store it */
  while(t_lbl->link) t_lbl = t_lbl->link;

  char *temp_buff = calloc(1, STR_LEN);
  printf("Name label %d: ", *lbl_count+1);
  getstring(temp_buff, STR_LEN, 'u');

  label *temp = malloc(sizeof(label));
  temp->max_len = 0;
  temp->fx = fxs+1;
  strcpy(temp->l_name, temp_buff);
  temp->max_len = UPDATE_MAX_LEN(temp_buff, temp->max_len);
  temp->link=NULL;
  t_lbl->link = temp;

  char *expr_str = malloc(STR_LEN);
       
  fputs("Formulae: ", stdout);
  getstring(expr_str, STR_LEN, 'n');
  /* storing a formulea indicator in a non standard way for future refs. */
  int size = strlen(temp_buff);
  
  /* resolve label names to corresponding data and create expression */
  char expr_resolved[STR_LEN];
  int expr_len = strlen(expr_str);
  short init=1;
  label *temp_lbl = *lbl;
  while(t_list)
  {
    bzero(expr_resolved, STR_LEN);
  
    if(expr_resolver(t_list, temp_lbl, expr_str, expr_resolved, *lbl_count, expr_len))
    { 
      hndl_user_err(LBL_NOT_FOUND);
      free(temp);
      t_lbl->link = NULL;
      return;
    }
    /*printf("%s\n", expr_resolved);
    puts("press");
    getchar();*/
    char postfx[2*expr_len+1]; /* length of the postfx buffer is kept such so to use space as delimiters */
         bzero(postfx, 2*expr_len+1);
    if(to_postfix(expr_resolved, postfx, 2*expr_len+1))
    {
      free(temp);
      t_lbl->link = NULL;
      return;
    }

    if(init)
    {
      rec *prev_node=NULL, *keep=NULL;
      while((*tlist))
      {
        /* allocate new space for storing ptrs to new labels */
        *tlist = realloc(*tlist, sizeof(rec) + ((*lbl_count+1) * sizeof(char *)));
        if(!(*tlist))
          hndl_fatal_err("malloc");

        if(prev_node) prev_node->link=*tlist;
        if(!keep) keep = *tlist;

        prev_node = *tlist;
        (*tlist) = (*tlist)->link;
      }
      *tlist = keep;
      t_list = *tlist;

      init=0;
    }
    float temp_res = evaluate(postfx, 2*expr_len+1);

    char *to_char = malloc(STR_LEN/2 * sizeof(char));
    sprintf(to_char, "%g", temp_res);
    t_list->rec_col_ptr[col_indx] = to_char;

    t_list = t_list->link;
  }
  formulaes[fxs] = expr_str;
  fxs++;

  (*lbl_count)++;
}

void del_col(rec **tlist, label **lbl, int *lbl_count, int pos)
{
  label *trav=*lbl;
  int indx=0;
  label *free_node;

  if(!*tlist) { hndl_user_err(REC_EMPT); return; }
  if(pos > *lbl_count-1) { hndl_user_err(INVALID_POS); return; }

  if(pos)
  {
    while(indx++ < pos-1 && trav)
      trav = trav->link;
     
    free_node = trav->link;
    trav->link = trav->link->link;

    if(free_node->fx)
    {
      free(formulaes[free_node->l_name[STR_LEN-1]]);
      fxs--;
    }

    free(free_node);
  }
  else /* if its the first node */
  {
    free_node = *lbl;
    if((*lbl)->link) *lbl = (*lbl)->link;
    else *lbl=NULL;
    if(free_node->fx)
    {
      free(formulaes[free_node->l_name[STR_LEN-1]]);
      fxs--;
    }
    free(free_node);
  }

  rec *prev_node=NULL, *keep=NULL;
  while((*tlist))
  {
    indx = pos;
    char *temp = (*tlist)->rec_col_ptr[pos];
    while(indx < *lbl_count-1)  {
      (*tlist)->rec_col_ptr[indx] = (*tlist)->rec_col_ptr[indx + 1];
      indx++;
    }
    free(temp);

    *tlist = realloc(*tlist, sizeof(rec) + ((*lbl_count - 1) * sizeof(char *)));
    if(!(*tlist)) { hndl_fatal_err("malloc"); }

    if(!keep) keep = *tlist;
    if(prev_node) prev_node->link = *tlist;
    prev_node = *tlist;
    *tlist = (*tlist)->link;
  }
  *tlist = keep;
  (*lbl_count)--;
}

void del_row(rec **list, char op, label *lbl, int lbl_count, int pos)
{
  rec *temp=*list;
  rec *prev=temp;

  if(!*list) { hndl_user_err(REC_EMPT); return; }
  if(op == 'b')
  {
    *list = temp->link;
    /* why not *list = *list->link ?? */
  }
  if(op == 'e')
  {
    if(!temp->link) { free(temp); *list = NULL; return; }
    while(temp->link)
    {
      prev = temp;
      temp = temp->link;
    }
    prev->link = NULL;
  }
  if(op == 'a')
  {
    int i=0;
    rec *t=NULL;

    if(!pos) { t = *list; *list = temp->link; }
    else 
    {
      while(temp->link && i++<pos)
        if(i < pos) temp = temp->link;
      if(!temp->link)  {
        hndl_user_err(INVALID_POS);
        return;
      }
      t = temp->link;
      temp->link = t->link;
    }
    temp = t;
  }
  printf("%s\nDeleting record:%s", yellow, norm);
  list_disp(temp, lbl, 's');
  puts("\nPress any key");
  getchar();
  freemem(temp, lbl, lbl_count, 's');
  puts("");
}

void edit_field(rec *tlist, label *lbl, char *row, char *col, char mode)
{
  short match_f=0;
  int indx=0;
  char temp_buff[STR_LEN];
  label *prev_lbl=NULL;

  while(lbl)
  {
    if(!strcmp(lbl->l_name, col))
    { match_f=1; break; }
    prev_lbl = lbl;
    lbl = lbl->link;
    indx++;
  }
  if(!match_f) {
    printf("\n%sLabel doesn't exist.%s\n\n", red, norm);
    puts("Press Enter");
    getchar();
    return; }
 
  if(mode == 'l')
  {
    printf("\nPrevious name: %s%-*s%s\n", yellow, max_label_len, lbl->l_name, norm);
    printf("\nNew name: ");
    getstring(temp_buff, STR_LEN, 'u');
    
    lbl = realloc(lbl, sizeof(label) + strlen(temp_buff) + 2);
    if(prev_lbl) prev_lbl->link = lbl;
    strcpy(lbl->l_name, temp_buff);
    lbl->max_len = UPDATE_MAX_LEN(temp_buff, lbl->max_len);
    return;
  }

  if(lbl->fx) fputs("\nThis is classified as a formulae field. Edit anyway? (y/n): ", stdout);
  char resp[2];
  getstring(resp, 2, 'n');
  if(resp[0] == 'n') return;

  match_f=0;
  while(tlist)
  {
    if(!strcmp(tlist->rec_col_ptr[0], row))
    { match_f=1; break; }
    tlist = tlist->link;
  }
  if(!match_f) { hndl_user_err(FIELD_NOT_FOUND); return;  }
  
  printf("\nPrevious value:\n%s%-*s%s: %s\n", yellow, max_label_len, lbl->l_name, norm,\
                                             tlist->rec_col_ptr[indx]);
  printf("\nEnter new value:\n%s%-*s%s: ", yellow, max_label_len, lbl->l_name, norm);
  getstring(temp_buff, STR_LEN, 'n');

  char *data = malloc(strlen(temp_buff));
  strcpy(data, temp_buff);
  lbl->max_len = UPDATE_MAX_LEN(temp_buff, lbl->max_len);
  free(tlist->rec_col_ptr[indx]);
  tlist->rec_col_ptr[indx] = data;
}

void srch_record(rec *tlist, label *lbl)
{
  if(!tlist || !lbl) { hndl_user_err(REC_EMPT); return; }

  rec *t_list=tlist;
  label *t_lbl=lbl;
  short int match_f=0;
  char mode[STR_LEN], str_key[STR_LEN];
  int num_key, col_indx=0;
  
  fputs("Available fields: ", stdout);
  while(t_lbl) 
    { printf("%s%c", t_lbl->l_name, t_lbl->link ? ',' : ' '); t_lbl=t_lbl->link; }
  fputs("\nSearch by: ", stdout);
  getstring(mode, STR_LEN, 'n');

  t_lbl=lbl;
  /* find if the field exists at all */
  while(t_lbl)
  {
    if(!strncmp(mode, t_lbl->l_name, strlen(t_lbl->l_name)))
      { match_f=1; break; }
    t_lbl = t_lbl->link;
    col_indx++;
  }
  if(!match_f) { hndl_user_err(FIELD_NOT_FOUND); return; }
  match_f=0;

  printf("%s: ", t_lbl->l_name);
  getstring(str_key, STR_LEN, 'n');
  
  fputs("\nMatches found: ", stdout); 
  while(t_list)
  {
    if(!strncmp(t_list->rec_col_ptr[col_indx], str_key, strlen(str_key)))
      { list_disp(t_list, lbl, 's'); match_f=1; }

    t_list = t_list->link;
  }

  if(!match_f) printf("%sNone%s\n\n", red, norm); 
  else puts("");

  puts("Press Enter");
  getchar();
}
 
void sort_record(rec **tlist, label *lbl, int lbl_count)
{
  if(!*tlist) { hndl_user_err(REC_EMPT); return; }

  label *t_lbl = lbl;
  fputs("Available fields: ", stdout);
  while(t_lbl)
  {
    printf("%s%c", t_lbl->l_name, t_lbl->link ? ',' : ' ');
    t_lbl = t_lbl->link;
  }

  t_lbl=lbl;
  char sort_as_lbl[STR_LEN];
  int col_indx=0, match_f=0;

  fputs("\nSort by: ", stdout);
  getstring(sort_as_lbl, STR_LEN, 'n');
  /* find out if the label exists */
  while(t_lbl)
  {
    if(!strncmp(sort_as_lbl, t_lbl->l_name, strlen(t_lbl->l_name)))
      { match_f=1; break; }
    t_lbl = t_lbl->link;
    col_indx++;
  }
  if(!match_f) { hndl_user_err(FIELD_NOT_FOUND); return; }
  int order=55;
  while(order!=1 && order!=2) {
    fputs("1.Ascending 2.Descending: ", stdout);
    getnums(&order);
  }
  rec *t_list=*tlist;
  rec *sorted_list=NULL;
  rec *temp=NULL, *temp2=NULL;
  rec *key = t_list;
  /* begin deletion sort */
  while(1)
  {
    key = t_list;
    temp2 = t_list;
    while(temp2)
    {
      int res=strcmp(temp2->rec_col_ptr[col_indx], key->rec_col_ptr[col_indx]);
      if(order==1) 
       { if(res<=0) key=temp2; }
      
      else if(order==2)
          { if(res>=0) key=temp2; }

      temp2 = temp2->link;
    }
    temp2=t_list;
    /* find node before key unless the darn 1st node is the key */
    while(key != t_list && temp2->link != key)
      temp2=temp2->link;
    /* special case if 1st node is key*/
    if(key != t_list) temp2->link = key->link;
    else t_list = t_list->link;

    key->link=NULL;
    if(!sorted_list) { sorted_list = key; temp=key; }
    else { temp->link = key; temp=key; }

    if(!t_list->link) break;
  }

  t_list->link=NULL;
  temp->link = t_list; /* the last node */
  *tlist=sorted_list;
}

/* modes: t = traversal, s = single node display */
void list_disp(rec *tlist, label *lbl, char mode)
{
  int col_indx=0, count=0;;
  label *t_lbl=lbl;
  if(!lbl) { hndl_user_err(REC_EMPT); return; }
  puts("");
  
  if(mode == 't') printf("Current table:\n");
  while(t_lbl) {
    printf("%s%-*s%s ", yellow, t_lbl->max_len, t_lbl->l_name, norm);
    t_lbl = t_lbl->link;
  }
  t_lbl=lbl;
  puts("");
  while(tlist)
  { 
    while(t_lbl) { 
      printf("%-*s ", t_lbl->max_len, (char *)tlist->rec_col_ptr[col_indx++]);
      t_lbl = t_lbl->link;
    }
    if(mode == 's') break;
    col_indx=0;
    t_lbl=lbl;
    tlist = tlist->link;
    puts("");
  }
  puts("");
}

void write_record(rec *tlist, label *lbl, int lbl_count, char *f_name, char format)
{
  if(!tlist) { hndl_user_err(REC_EMPT); return; }

  FILE *out=NULL;
  label *t_lbl=lbl;
  char ch[2];
  rec *temp=tlist;
  int w_fx=0;
  if(!access(f_name, F_OK)) 
  { 
    printf("%sFile already exists. Overwrite? :(y/n) %s", red, norm);
    getstring(ch, 2, 'n');
    if((ch[0] == 'n')) return;
  }
  if(!(out = fopen(f_name, "w"))) { hndl_user_err(FAILD_TO_OPEN_STRM); return; }
  if(format == 'p')
  {
    fprintf(out, "%d %u\n\n", lbl_count, max_label_len);
    while(t_lbl)
    {
      fprintf(out, "%s %d %s%s", t_lbl->l_name, t_lbl->max_len,\
      t_lbl->fx ? formulaes[t_lbl->fx-1] : "n",\
      t_lbl->link ? "\n" : "\n\n");

      t_lbl = t_lbl->link;
    }
    t_lbl=lbl;
    int indx;
    while(temp) 
    {
      indx=0;
      fprintf(out, "[START]\n");
      while(t_lbl)
      {
        fprintf(out, "%s=%s\n", t_lbl->l_name, temp->rec_col_ptr[indx++]);
        t_lbl = t_lbl->link;
      }
      fprintf(out, "[END]\n\n");
      temp = temp->link;
      t_lbl=lbl;
    }
  }
  else
  {
    int indx;
    while(t_lbl)
    {
      fprintf(out, "%-*s", max_label_len, t_lbl->l_name);
      t_lbl = t_lbl->link;
    }
    fputs("\n", out);
    while(temp)
    {
      indx=0;
      while(indx<lbl_count)
        fprintf(out, "%-*s", max_label_len, temp->rec_col_ptr[indx++]);
      fputs("\n", out);
      temp=temp->link;
    }
  }

  printf("Saved record to file '%s' on disk.\n\n", f_name);
  fclose(out);
  usleep(959999U);
  return;
}

char *get_line(char *looper, char *buff)
{
  int indx=0;
  while(*looper!='\n' && indx<MAX_FILE_LINE_LEN)
    buff[indx++] = *looper++;
  buff[indx] = '\0';
  return looper;
}

void read_record(rec **tlist, label **lbl, int *lbl_count, char *f_name)
{
  FILE *in;
  char *mem=NULL;
  register char *looper=NULL;
  char temp_buff[MAX_FILE_LINE_LEN], ch;

  memset(temp_buff, '\0', MAX_FILE_LINE_LEN);

  if(!(in = fopen(f_name, "r"))) { hndl_user_err(FAILD_TO_OPEN_STRM); return; }

  fseek(in, 0L, SEEK_END);
  long bytes=ftell(in);
  rewind(in);

  if(bytes <= 1)  {
    printf("\n%sFile is empty.%s\n\nPress any key.\n", red, norm);
    getchar();
    return;
   }

  if(!(mem=malloc(sizeof(char) * bytes)))
    hndl_fatal_err("malloc");
  looper = mem;
  fread(looper, 1, bytes, in);

  int indx;
  char *tokens[2]={ NULL }, *temp=NULL, *token="ha";

  looper = get_line(looper, temp_buff);
  temp = temp_buff;
  indx=0;
  /* take in no of labels and max_label_len */
  while(token && indx<2)
  {
    token=strtok(temp, " ");
    tokens[indx++] = token;
    temp=NULL;
  }
  /* a bit of parsability checking */
  if(tokens[0] && tokens[1])
  {
    if(!isdigit(*tokens[0]) || !isdigit(*tokens[1]))
      { hndl_user_err(ABORT_PARSE); free(mem); fclose(in); return; }
  }
  else
  { hndl_user_err(ABORT_PARSE); free(mem); fclose(in); return; }
  /* no further format comatibility is tested after this point. so erase exisiting
     record. If any.                                                            */
  if(*tlist) freemem(*tlist, *lbl, *lbl_count, 'a');
  *lbl=NULL, *tlist=NULL;

  *lbl_count=atoi(tokens[0]); max_label_len=atoi(tokens[1]);

  looper += 2;

  int got_labels=0;
  /* create labels */
  label *prev_lbl_node=NULL;
  fxs=0;
  while(got_labels<*lbl_count && *looper)
  {
    looper = get_line(looper, temp_buff);
    
    char *tokens[3] = { NULL };
    temp=temp_buff;
    token=temp_buff;
    
    indx=0;
    while(token && indx<3)
    {
      token=strtok(temp, " ");
      tokens[indx++] = token;
      temp=NULL;
    }

    if(!tokens[0]) tokens[0] = "";
    if(!tokens[2]) tokens[2] = "";

    label *temp; 
    
    size_t alloc_size = strlen(tokens[2]);

    if(!(temp = malloc(sizeof(label) + alloc_size+2))) 
      hndl_fatal_err("malloc");

    if(!*lbl) *lbl=temp;
    strcpy(temp->l_name, tokens[0]);

    if(!tokens[1]) temp->max_len = strlen(tokens[0]);
    else temp->max_len = atoi(tokens[1]);

    if(strcmp(tokens[2],"n"))
    {
       char *expr_str=malloc(strlen(tokens[2])+1);
       strcpy(expr_str, tokens[2]);
       formulaes[fxs++] = expr_str;
       temp->fx = fxs;
    }
    else temp->fx = 0;

    if(prev_lbl_node) prev_lbl_node->link = temp;
    temp->link=NULL;
    prev_lbl_node = temp;
    got_labels++;
    looper++;
    bzero(temp_buff, MAX_FILE_LINE_LEN);
  }

  looper++;
  /* create entries */
  rec *prev_rec_node=NULL;
  rec *temp_rec;
  char *empty_str="";
  while(*looper)
  {
    indx=0;
    int col_indx=0;
    looper = get_line(looper, temp_buff);

    if(!strcmp(temp_buff, "[START]"))
    {
      looper++;

      if(!(temp_rec = malloc(sizeof(rec) + (*lbl_count * sizeof(char *))))) 
        hndl_fatal_err("malloc");
      if(!*tlist) *tlist = temp_rec;

      while(1)
      {
        indx=0;
        bzero(temp_buff, MAX_FILE_LINE_LEN);
        looper = get_line(looper, temp_buff);
        if(!strcmp(temp_buff, "[END]"))
          { looper += 2; break; }

        char *tokens[2]={ NULL }, *temp_holder=temp_buff, *token="ha";
        
        while(token && indx<2)
        {
          token = strtok(temp_holder, "=");
          tokens[indx++] = token;
          temp_holder=NULL;
        }
        
        tokens[1] = tokens[1] ? tokens[1] : empty_str;
         
        char *data = malloc(strlen(tokens[1]));
        strcpy(data, tokens[1]);
        temp_rec->rec_col_ptr[col_indx++] = data;

        looper++;
      }
    }
    if(prev_rec_node) prev_rec_node->link = temp_rec;
    prev_rec_node = temp_rec;
  }
  temp_rec->link = NULL;

  fclose(in);
  free(mem);
}

void freemem(rec *tlist, label *lbl, int lbl_count, char mode)
{
  int i=0;
  rec *tmp_list=NULL;

  while(tlist)  {
    tmp_list = tlist;
    for(; i<lbl_count; i++)
      free(tmp_list->rec_col_ptr[i]);
    i=0;
    tlist = tlist->link;
    free(tmp_list);
    if(mode == 's') return; /* free mem of a single row */
  }
  label *tmp_lbl=NULL;
  while(lbl)
  {
    tmp_lbl = lbl;
    lbl = lbl->link;
    free(tmp_lbl);
  }
}

int main(void)
{
  rec *list = NULL;
  label *lbl = NULL;
  int resp;
  int lbl_count=0;
  
  struct op {
    char *mne;
    short num;
  };
  system("clear");
  
  printf("%s\n\
   ______                    _____ __              __\n\
  / ____/___  ____ ___      / ___// /_  ___  ___  / /_\n\
 / /   / __ \\/ __ `__ \\     \\__ \\/ __ \\/ _ \\/ _ \\/ __/\n\
/ /___/ /_/ / / / / / /    ___/ / / / /  __/  __/ /_  \n\
\\____/\\____/_/ /_/ /_/____/____/_/ /_/\\___/\\___/\\__/\n\
                    /_____/                       v0.3%s\n\n", bcyan, norm);
  puts("Enter \"help\" to get a list of commands.");
  while(1)
  {
    char comm_queue[20];
    printf("%s==> %s", red, norm);
    getstring(comm_queue, 20, 'n');

    struct op opcodes[19] = { "irb", 1, "ire", 2, "ira", -3,
                              "drb", 4, "dre", 5, "dra", -6,
                              "ica", -7, "dca", -8, 
                              "sch", 9, "srt", 10, "help", 11,
                              "cls", 12, "wrt", -13, "wrp", -14,
                              "rdf", -15, "edf", -16, "edl", -17,
                              "icf", 18, "q", 0 };
    unsigned short indx=0;
    int code=-99, pos;
    char *parse=comm_queue;
    char *temp="ha";
    char *tokens[3] = { NULL };

    while(temp)
    {
      temp = strtok(parse, " ");
      if(temp) tokens[indx++] = temp;
      parse=NULL;
    }

    if(!tokens[0]) continue;

    if((indx>=2 && temp))
      { hndl_user_err(WRONG_COMM_ERR); continue; }

    for(indx=0; indx<19; indx++)
    {
      if(!strcmp(tokens[0], opcodes[indx].mne))
        { code = opcodes[indx].num; break; }
    }
    if(code == -99) { hndl_user_err(WRONG_COMM_ERR); continue; }
    if(code < 0)
    {
      if(tokens[1])
      {
        if(strstr(tokens[0], "wrtwrprdf")) ;
        else if(code == -16) /* edt */
        {
          if(!tokens[2]) /* we need row and column name */
          { hndl_user_err(WRONG_COMM_ERR); continue; }
        }
        else if(isdigit(*tokens[1]))
          pos = atoi(tokens[1])-1;
        else if(*tokens[1] == 'b')
          pos=0;
        else if(*tokens[1] == 'e')
        {
          switch(code) {
            case -3: code=2; break; /* ira :: irb */
            case -6: code=5; break; /* dra :: drb */
            case -7: pos=lbl_count; break; /* ica */
            case -8: pos=lbl_count-1; break; /* dca */
          }
        }
      }
      else { hndl_user_err(WRONG_COMM_ERR); continue; }
    }
 /*   printf("%s : %d %d\n", tokens[0], code, pos);*/
    code = code < 0 ? -(code) : code;

    switch(code)
    {
        case 1: ins_row_beg(&list, &lbl, &lbl_count);
                break;
        case 2: ins_row_end(&list, NULL, &lbl, &lbl_count);
                break;
        case 3: ins_row_any(&list, &lbl, &lbl_count, pos);
                break;
        case 4: del_row(&list, 'b', lbl, lbl_count, 0);
                break;
        case 5: del_row(&list, 'e', lbl, lbl_count, 0);
                break;
        case 6: del_row(&list, 'a', lbl, lbl_count, pos);
                break;
        case 7: ins_col(&list, &lbl, &lbl_count, pos, 1);
                break;
        case 8: del_col(&list, &lbl, &lbl_count, pos);
                break;
        case 9: srch_record(list, lbl);
                break;
        case 10: sort_record(&list, lbl, lbl_count);
                break;
        case 11: show_help();
                 break;
        case 12: system("clear");
                 break;
        case 13: write_record(list, lbl, lbl_count, tokens[1], 't');
                 break;
        case 14: write_record(list, lbl, lbl_count, tokens[1], 'p');
                 break;
        case 15: read_record(&list, &lbl, &lbl_count, tokens[1]);
                 break;
        case 16: edit_field(list, lbl, tokens[1], tokens[2], 'r');
                 break;
        case 17: edit_field(list, lbl, NULL, tokens[1], 'l');
                 break;
        case 18: ins_formulae_col(&list, &lbl, &lbl_count);
                 break;
        case 0:
          freemem(list, lbl, lbl_count, 'a');
          puts("\n\tYou exited Com_Sheet v0.3.\n\tHave a nice day!\n");
          exit(EXIT_SUCCESS);
    }
      if(list && code != 11) { system("clear"); list_disp(list, lbl, 't'); }
      bzero(comm_queue, 10);
      tokens[0]=NULL; tokens[1]=NULL, tokens[2]=NULL;
  }
}

