/**************************************************************/
/* ENSEMBLE.C */
/* Author: Mark Hayden, 11/99 */
/* Copyright 1999 Cornell University.  All rights reserved. */
/* Copyright 1999, 2000 Mark Hayden.  All rights reserved. */
/* See license.txt for further information. */
/**************************************************************/

static string_t name = "ENSEMBLE" ;

type tcontrol = 
  | TLeave
  | TPrompt
  | TSuspect of rank list
  | TXferDone 
  | TRekey
  | TProtocol of Proto.id
  | TMigrate of Addr.set
  | TTimeout of Time.t
  | TDump
  | TBlock of bool

type taction =
  | TCast of string
  | TSend of string * string
  | TControl of tcontrol

let read_lines () =
  let stdin = Hsys.stdin () in
  let buf = String.create 10000 in
  let len = Hsys.read stdin buf 0 (String.length buf) in
  if len = 0 then raise End_of_file ;
  let buf = String.sub buf 0 len in
  let rec loop buf =
    try 
      let tok,buf = strtok buf "\n" in
      if tok = "" then (
	loop buf 
      ) else (
	tok :: loop buf
      )
    with Not_found -> []
  in
  loop buf

/* This function returns the interface record that defines
 * the callbacks for the application.
 */
let intf endpt alarm sync =

  /* This is the buffer used to store typed input prior
   * to sending it.
   */
  let buffer 	= ref [] in
  
  /* View_inv is a hashtable for mapping from endpoints to ranks.
   */
  let view_inv	= ref (Hashtbl.create 10) in
  let view	= ref [||] in

  /* This is set below in the handler of the same name.  Get_input
   * calls the installed function to request an immediate callback.
   */
  let async_r = ref (fun () -> ()) in
  let async () = !async_r () in

  let action act =
    buffer := act :: !buffer ;
    async ()
  in

  /* Handler to read input from stdin.
   */
  let prev_buf = ref "" in
  let get_input () =
    let buf = String.create 4096 in
    let len = Hsys.read (stdin()) buf 0 (String.length buf) in
    if len = 0 then exit 0 ;
    let buf = String.sub buf 0 len in
    let lines =
      let rec loop buf =
	if not (String.contains buf '\n') then (
	  prev_buf := !prev_buf ^ buf ;
	  []
	) else (
	  let buf = !prev_buf ^ buf in
	  prev_buf := "" ;
	  try 
	    let line,buf = strtok buf "\n" in
	    line :: loop buf
	  with Not_found -> 
	    /* It must have been all '\n's.
	     */
	    []
	)
      in loop buf
    in

    List.iter (fun line ->
      eprintf "line=%s\n" line ;
      let com,rest = 
	try strtok line " \t" with
	  Not_found -> ("","")
      in
      match com with
      |	"" -> ()
      | "cast" ->
	  action (TCast(rest))
      | "send" -> 
	  let dest,rest = strtok rest " \t" in
	  begin try Hashtbl.find !view_inv dest with
	  | Not_found ->
	      eprintf "ENSEMBLE:bad endpoint name:'%s'\n" dest ;
	      failwith "bad endpoint name"
	  end ;
	  action (TSend(dest,rest))
      | "leave" ->
	  action (TControl TLeave)
      | "prompt" ->
	  action (TControl TPrompt)
      | "suspect" -> 
	  let susp,_ = strtok rest " \t" in
	  let susp = int_of_string susp in
	  action (TControl (TSuspect [susp]))
      | "xferdone" -> 
	  action (TControl TXferDone)
      | "rekey" -> 
	  action (TControl TRekey)
      | "protocol" -> 
	  let proto,_ = strtok rest " \t" in
	  let proto = Proto.id_of_string proto in
	  action (TControl (TProtocol proto))
      | "migrate" -> 
	  failwith "Currently not supported"
      | "timeout" -> 
	  let time,_ = strtok rest " \t" in
	  let time = float_of_string time in
	  let time = Time.of_float time in 
	  action (TControl (TTimeout time))
      | "block" ->
	  let blk,_ = strtok rest " \t" in
	  let blk = bool_of_string blk in
	  action (TControl (TBlock blk))
      | other -> 
	  eprintf "bad command '%s'\n" other;
	  eprintf "commands:\n" ;
	  eprintf "  cast <msg>           : Cast message\n" ;
	  eprintf "  send <dst> <msg>     : Send pt-2-pt message\n" ;
	  eprintf "  leave                : Leave the group\n" ;
	  eprintf "  prompt               : Prompt for a view change\n" ;
	  eprintf "  suspect <suspect>    : Suspect a member\n" ;
	  eprintf "  xferdone             : State transfer is complete\n" ;
	  eprintf "  rekey                : rekey the group\n" ;
	  eprintf "  protocol <proto_id>  : switch the protocol\n" ;
	  eprintf "  migrate <address>    : migrate to a new address\n" ;
	  eprintf "  timeout <time>       : request a timeout\n" ;
	  eprintf "  block   <true/false> : block (yes/no)\n" ;
	  exit 1 ;
    ) lines
  in
  Alarm.add_sock_recv alarm name (Hsys.stdin()) (Handler0 get_input) ;

  /* If there is buffered data, then return an action to
   * send it.
   */
  let check_actions () =
    let actions = ref [] in
    List.iter (function
      | TCast msg -> 
      	  actions := Cast msg :: !actions
      | TSend(dest,msg) -> (
	  try 
      	    let dest = Hashtbl.find !view_inv dest in
	    actions := Send1(dest,msg) :: !actions
	  with Not_found -> ()
      	)
      | TControl tc -> 
	  let action = match tc with 
	    | TLeave -> Leave 
	    | TPrompt -> Prompt
	    | TSuspect rl -> Suspect rl
	    | TXferDone  -> XferDone
	    | TRekey -> Rekey false
	    | TProtocol p -> Protocol p
	    | TMigrate addr -> Migrate addr
	    | TTimeout t -> Timeout t
	    | TDump -> Dump
	    | TBlock b -> Block b
	  in
      	  actions := Control action :: !actions
    ) !buffer ;
    buffer := [] ;
    Array.of_list !actions
  in

  /* Print out the name.
   */
  printf "endpt %s\n" (Endpt.string_of_id endpt) ;

  let install (ls,vs) =
    async_r := Appl.async (vs.group,ls.endpt) ;
    view := Array.map Endpt.string_of_id (Arrayf.to_array vs.view) ;
    view_inv := Hashtbl.create 10 ;
    printf "view %d %d %s\n"
      ls.nmembers ls.rank
      (String.concat " " (Array.to_list !view)) ;
    for i = 0 to pred ls.nmembers do
      Hashtbl.add !view_inv !view.(i) i
    done ;


void main_receive(
        env_t env,
	rank_t from,
	is_send_t is_send,
	blocked_t blocked,
	iovec_t iov
) {
    const char *cast_send ;
    if (is_send) {
	cast_send = "send" ;
    } else {
	case_send = "cast" ;
    }
    printf("%s %d %s", cast_send, from, "GORP") ;
    iovec_free(iov) ;
}

static
void main_heartbeat(env_t env, etime_t time) {
}

static
void main_block(env_t env) {
    /* BUG: Add sync support.
     */
    printf "block\n" ;
}

static
void main_disable(env_t env) {
}

void main_exit(env_t env) {
    printf("exit\n") ;
    exit(0) ;
}

int main(int argc, char *argv[]) {
    view_state_t vs ;
    unique_t unique ;
    group_id_t group ;
    endpt_id_t endpt ;
    proto_id_t proto ;
    addr_id_t addr ;
    sched_t sched ;
    alarm_t alarm ;
    int i ;

    sched = sched_create(name) ;
    unique = unique_create(sys_gethost(), sys_getpid()) ;
    alarm = real_init(sched, unique) ;
    appl_init() ;
    group = group_named(name) ;
    udp_init(alarm) ;
    addr = domain_addr(domain, ADDR_UDP) ;

    proto = proto_id_of_string("BOTTOM:MNAK:PT2PT:PT2PTW:"
			       "TOP_APPL:STABLE:VSYNC:SYNC:"
			       "ELECT:INTRA:INTER:LEAVE:SUSPECT:"
			       "PRESENT:HEAL:TOP") ;

    for (i=0;i<NPROCS;i++) {
	layer_state_t state = record_create(layer_state_t, state) ;
	state_t s = record_create(state_t, s) ;
	state->appl_intf = main_appl() ;
	state->appl_intf_env = s ;
	state->alarm = alarm ;
	endpt = endpt_id(unique) ;
	vs = view_singleton(proto, group, endpt, addr, LTIME_FIRST, time_zero()) ;
	s->ls = NULL ;
	s->vs = NULL ;
	layer_compose(state, &endpt, vs) ;
    }
    
    appl_mainloop(alarm) ;
    return 0 ;
}
   
