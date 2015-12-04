/**************************************************************/
/* FRAG.C */
/* Author: Mark Hayden, 11/99 */
/* There is something of a hack for the interaction between
 * this and the Mflow layer.  This layer marks all
 * fragmented messages with the Fragment flag.  This forces
 * the message to be subject to flow control even if the
 * event is not from the application.  Otherwise, the Mflow
 * layer may reorder application and non-application
 * fragments, causing problems. */
/**************************************************************/
/* FRAG.C */
/* Author: Mark Hayden, 11/99 */
/**************************************************************/
#include "infr/util.h"
#include "infr/layer.h"
#include "infr/view.h"
#include "infr/event.h"
#include "infr/trans.h"

static string_t name = "FRAG" ;

typedef enum { NOHDR, FRAG, LAST } header_type_t ;
	
typedef struct state_t {
    layer_t layer ;
    view_local_t ls ;
    view_state_t vs ;
    array_of(frag_t) send ;
    array_of(frag_t) cast ;
    bool_array_t local ;
    len_t max_len ;
} *state_t ;

#include "infr/layer_supp.h"

static void dump(state_t s) {
/*
  sprintf "my_rank=%d, nmembers=%d\n" ls.rank ls.nmembers ;
  sprintf "view =%s\n" (View.to_string vs.view) ;
  sprintf "local=%s\n" (Arrayf.to_string string_of_bool s.local)
*/
    abort() ;
}

static void init(
        state_t s,
        layer_t layer,
	layer_state_t state,
	view_local_t ls,
	view_state_t vs
) {
    s->ls = ls ; 
    s->vs = vs ;
    s->layer = layer ;
    s->local = bool_array_create(vs->nmembers, FALSE) ;
    s->cast = array_create(frag_t, vs->nmembers) ;
    s->send = array_create(frag_t, vs->nmembers) ;
    for (ofs=0;ofs<vs->nmembers;ofs++) {
	frag_t *f ;
	f = &array_get(s->cast, ofs) ;
	f->i = 0 ;
	f->iov = NULL ;
	f = &array_get(s->send, ofs) ;
	f->i = 0 ;
	f->iov = NULL ;
    }
    return s ;
}

static void up_handler(state_t s, event_t e, unmarsh_t abv) {
    switch (event_type(e)) {
    default:
	up(s, e, abv) ;
	break ;
    }
}

static void uplm_handler(state_t s, event_t e, unmarsh_t msg, iovec_t iov) {
    switch (event_type(e)) {
    default:
	abort() ;
	break ;
    }
}

static void upnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {

    EVENT_DUMP_HANDLE() ;

    default:
	upnm(s, e) ;
	break ;
    }
}

static void dn_handler(state_t s, event_t e, marsh_t abv) {
    switch(event_type(e)) {
    default:
	dn(s, e, abv) ;
	break ;
    }
}

static void dnnm_handler(state_t s, event_t e) {
    switch(event_type(e)) {
    default:
	dnnm(s, e) ;
	break ;
    }
}

static void free_handler(state_t s) {
    array_free(s->failures) ;
}

void frag_register(void) {
    layer_register(name, init, up_handler, uplm_handler, upnm_handler,
		   dn_handler, dnnm_handler, dump) ;
}

type frag = {
  mutable iov : Iovecl.t array ;
  mutable i : int
}

/* The last fragment also has the protocol headers
 * for the layers above us.
 */
type header = NoHdr 
  | Frag of seqno * seqno		/* ith of n fragments */
  | Last of seqno			/* num of fragments */

let dump = Layer.layer_dump name (fun (ls,vs) s -> [|
  sprintf "my_rank=%d, nmembers=%d\n" ls.rank ls.nmembers ;
  sprintf "view =%s\n" (View.to_string vs.view) ;
  sprintf "local=%s\n" (Arrayf.to_string string_of_bool s.local)
|])

let init _ (ls,vs) = 
  let local_frag = Param.bool vs.params "frag_local_frag" in
  let addr = ls.addr in
  let local = Arrayf.map (fun a -> (not local_frag) && Addr.same_process addr a) vs.address in
  let all_local = Arrayf.for_all ident local in
  let log = Trace.log2 name ls.name in
  { max_len = len_of_int (Param.int vs.params "frag_max_len") ;
    cast = Array.init ls.nmembers (fun _ -> {iov=[||];i=0}) ;
    send = Array.init ls.nmembers (fun _ -> {iov=[||];i=0}) ;
    all_local = all_local ;
    local = local
  }

let hdlrs s ((ls,vs) as vf) {up_out=up;upnm_out=upnm;dn_out=dn;dnlm_out=dnlm;dnnm_out=dnnm} =
/*let ack = make_acker name dnnm in*/
  let log = Trace.log2 name ls.name in

  /* Handle the fragments as they arrive.
   */
  let handle_frag ev i n abv =
    /* First, flatten the iovec.  Normally, the iovec
     * should have only one element anyway.  
     */
    let iovl = getIov ev in
    let origin = getPeer ev in
    
    /* Select the fragment info to use.
     */
    let frag = 
      if getType ev = ECast
      then s.cast.(origin)
      else s.send.(origin)
    in

    /* Check for out-of-order fragments: could send up lost
     * message when they come out of order.  
     */
    if frag.i <> i then (
      log (fun () -> sprintf "expect=%d/%d, got=%d, type=%s\n" 
        frag.i (Array.length frag.iov) i (Event.to_string ev)) ;
      sys_panic "fragment arrived out of order" ;
    ) ;
    
    /* On first fragment, allocate the iovec array where
     * we will put the rest of the entries.
     */
    if i = 0 then (
      if frag.iov <> [||] then sys_panic sanity ;
      frag.iov <- Array.create n Iovecl.empty
    ) ;
    if Array.length frag.iov <> n then
      sys_panic "bad frag array" ;
    
    /* Add fragment into array.
     */
    if i >= Array.length frag.iov then
      sys_panic "frag index out of bounds" ;
    frag.iov.(i) <- iovl ;
    frag.i <- succ i ;
    
    /* On last fragment, send it up.  Note that the ack on
     * this should ack all previous fragments.
     */
    match abv with
    | None ->
	if i = pred n then sys_panic sanity ;
      	/*ack ev ;*/ free_noIov name ev
    | Some abv ->
    	if i <> pred n then sys_panic sanity ;
        let iov = Iovecl.concata name (Arrayf.of_array frag.iov) in
      	frag.iov <- [||] ;
      	frag.i <- 0 ;
      	up (set name ev [Iov iov]) abv
  in

  let up_hdlr ev abv hdr = match getType ev, hdr with
    /* Common case: no fragmentation.
     */
  | _, NoHdr -> up ev abv

  | (ECast | ESend), Last(n) ->
      handle_frag ev (pred n) n (Some abv)
  | _, _     -> sys_panic bad_up_event

  and uplm_hdlr ev hdr = match getType ev, hdr with
  | (ECast | ESend), Frag(i,n) ->
      handle_frag ev i n None
  | _ -> sys_panic unknown_local

  and upnm_hdlr ev = match getType ev with
  | EExit ->
      /* GC all partially received fragments.
       */
      let free_frag_array fa =
	Array.iter (fun f ->
	  Array.iter (fun i -> Iovecl.free name i) f.iov ;
          f.iov <- [||]
	) fa
      in

      free_frag_array s.send ;
      free_frag_array s.cast ;
      upnm ev

  | _ -> upnm ev
  
  and dn_hdlr ev abv = match getType ev with
  | ECast | ESend ->
      /* Messages are not fragmented if either:
       * 1) They are small.
       * 2) They are tagged as unreliable. (BUG?)
       * 3) All their destinations are within this process.
       */
      let iovl = getIov ev in
      let lenl = Iovecl.len name iovl in
      if lenl <=|| s.max_len
      || (getType ev = ECast && s.all_local)
      || (getType ev = ESend && Arrayf.get s.local (getPeer ev))
      then (
	/* Common case: no fragmentation.
	 */
      	dn ev abv NoHdr
      ) else (
	let iovl = Iovecl.copy name iovl in
/*
        log (fun () -> sprintf "fragmenting") ;
*/
	/* Fragment the message.
	 */
	let frags = Iovecl.fragment name s.max_len iovl in
	Iovecl.free name iovl ;

	let nfrags = Arrayf.length frags in
	assert (nfrags >= 2) ;
	for i = 0 to nfrags - 2 do
	  dnlm (setIovFragment name ev (Arrayf.get frags i)) (Frag(i,nfrags)) ;
	done ;

	/* The last fragment has the header above us.
         */
	dn (setIovFragment name ev (Arrayf.get frags (pred nfrags))) abv (Last(nfrags)) ;
	free name ev
      )
  | _ -> dn ev abv NoHdr

LAYER_REGISTER(frag) ;
