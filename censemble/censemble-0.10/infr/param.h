/**************************************************************/
/* PARAM.H */
/* Author: Mark Hayden, 4/00 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/
open Trans
/**************************************************************/

/* The type of parameters.
 */
type t =
  | String of string
  | Int of int
  | Bool of bool
  | Time of Time.t
  | Float of float

/* Parameter lists are (name,value) association lists.
 */
type tl = (name * t) list

/* Add a parameter to the defaults.
 */
val default : name -> t -> unit

/* Lookup a parameter in a param list.
 */
val lookup : tl -> name -> t

/* Lookup a particular type of parameter.
 */
string_t param_string(param_t, name_t) ; /* caller needs to free string */
int param_int(param_t, name_t) ;
bool_t param_bool(param_t, name_t) ;
time_t param_time(param_t, name_t) ;
float_t param_float(param_t, name_t) ;

#if 0
val to_string : (name * t) -> string

/* Print out default settings.
 */
val print_defaults : unit -> unit
#endif
