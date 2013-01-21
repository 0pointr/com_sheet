#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define DELIM ' '

typedef struct prece {
  char ch;
  int value;
  } prec;

int get_prece(char pop)
{
    const prec prece_table[5] = { '^', 2, '*', 1, '/', 1,
                                  '+', 0, '-', 0 };
    int indx=0;
    int pop_val=-1;

    while(indx<5)
    {
      if(prece_table[indx].ch == pop)
        { pop_val = prece_table[indx].value; break; }
      indx++;
    }
    
    return pop_val;
}

char deque(char *buff, int *tail)
{
  if(*tail == strlen(buff)) return -1;

  return buff[(*tail)++];
}

char pop(char *buff, int *top)
{
  if(*top == 0) return -1;
   
  return buff[--(*top)];
}

char push(char *buff, char item, int *top, int top_max)
{
  if(*top == top_max+1) return -1;

  buff[(*top)++] = item;
}

int to_postfix(char *work_buff, char *postfx, unsigned exp_len)
{
  
  char poped;
  char sym_buff[exp_len];
       bzero(sym_buff, exp_len);
  int work_tail=0;
  int post_top=0, sym_top=0;
  int curr_pop_val, last_push_val=-99;

  while((poped=deque(work_buff, &work_tail)) != -1)
  {
    if(isdigit(poped) || poped == '.') /* digits */
    {
      while((isdigit(poped) || poped == '.') && poped != -1)
      {
        push(postfx, poped, &post_top, exp_len);
        if(last_push_val == -99) last_push_val=-10;
        poped=deque(work_buff, &work_tail);
      }
      if(poped != -1) work_tail--;
      push(postfx, DELIM, &post_top, exp_len);
    }
    else if(poped == '(')
    {
      if(last_push_val == -99) last_push_val=-10;
      push(sym_buff, poped, &sym_top, exp_len);
    }
    else if(poped == ')')
    {
      if(last_push_val == -99) return __LINE__;

      char temp;

      while((temp = pop(sym_buff, &sym_top)) != '(' && temp != -1)
      {
        push(postfx, temp, &post_top, exp_len);
        push(postfx, DELIM, &post_top, exp_len);
      }
      
      if(temp != '(') return __LINE__; /* missing opening parenthesis */

    }
    else /* oparators */
    {
      if(last_push_val == -99) return __LINE__;

      if((curr_pop_val = get_prece(poped)) == -1) return __LINE__; 

      if(last_push_val < curr_pop_val) {
        push(sym_buff, poped, &sym_top, exp_len);
      }
      else
      {
        while(1)
        {
           char pop_higher;
           pop_higher = pop(sym_buff, &sym_top);

           if(pop_higher != -1)
           {
             int temp_prece;
             if((temp_prece=get_prece(pop_higher)) >= curr_pop_val)
             {
               if(temp_prece != -1) {
                 push(postfx, pop_higher, &post_top, exp_len);
                 push(postfx, DELIM, &post_top, exp_len);
               }
               else /* alien character */
                 return __LINE__;
             }
             else
             {
               /* last poped thing that has a lower precedence */
               sym_top++;
               break;
             }
           }
           else break;
        }
        push(sym_buff, poped, &sym_top, exp_len);
      }
      last_push_val = curr_pop_val;
    }
  }

  while((poped = pop(sym_buff, &sym_top)) && poped != -1)
  {
    if(poped == '(') return 1;
    push(postfx, poped, &post_top, exp_len);
    push(postfx, DELIM, &post_top, exp_len);
  }
  return 0;
}

float evaluate(char *exp, unsigned exp_len)
{
  char dequed;
  float ev_buff[exp_len];
  int indx=0, exp_tail=0;

  while((dequed=deque(exp, &exp_tail)) != -1)
  {
    if(dequed == DELIM) continue;

    if(isdigit(dequed) || dequed == '.')
    {
      ev_buff[indx++] = atof(&exp[exp_tail-1]);
      /* number lenght is unknown, forward upto next DELIM */
      while(dequed=(deque(exp, &exp_tail)) != DELIM && dequed != -1) ;
    }
    else
    {
      switch(dequed)
      {
        case '^':
          ev_buff[indx-2] = pow(ev_buff[indx-2], ev_buff[indx-1]);
          break;
        case '*':
          ev_buff[indx-2] = ev_buff[indx-2] * ev_buff[indx-1];
          break;
        case '/':
          ev_buff[indx-2] = ev_buff[indx-2] / ev_buff[indx-1];
          break;
        case '+':
          ev_buff[indx-2] = ev_buff[indx-2] + ev_buff[indx-1];
          break;
        case '-':
          ev_buff[indx-2] = ev_buff[indx-2] - ev_buff[indx-1];
          break;
      }
      indx--;
    }
  }
  return ev_buff[0];
}
