
open Core.Std
open Globals
open Algorithms

module type QUERY_RESPONDER = 
sig

    type hit 

    type aligned_sequence
    
    val list_my_objects : string -> container list -> int -> int -> string -> hit list

end

module Traverse_Sequences (S:SEQ_ALG) : (QUERY_RESPONDER with type aligned_sequence = S.aligned_sequence) = 
struct
    
   let header_markup = "<!DOCTYPE html>
<html xmlns='http://www.w3.org/1999/xhtml'>
<head>
<meta http-equiv='Content-Type' content='text/html; charset=UTF-8' />
<title>Sequence Alignment Output</title>
<link rel='stylesheet' href='css/biostat.css' type='text/css'>
</head><body><h1>Sequence Output</h1>"


   let footer_markup = "</body></html>"
   
   type aligned_sequence = S.aligned_sequence
 
(* the "hit" incorporates the aligned sequence and the container's comment.*)

   type hit = { myscore : int; sequences: (string*string);  comment : string}

   let match_sequences (s1:string) (s2:string) : aligned_sequence = 
     match String.uppercase (Sys.argv.(2)) with 
     | "LOCAL" -> S.align_local s1 s2
     | "REPEATS" -> S.align_repeats s1 s2
     | _  -> S.align_global s1 s2
    
   (*replaces spaces in string with nbsp's for html output.*)
 let nbsp str = 
let  string_to_list (str:string) : 'a list = 
  let rec help count lst = 
  if count >= String.length str then List.rev (lst) else
    let head = String.get str count in
    help (count+1) (head::lst)
  in help 0 [] in

  let to_nbsp str = 
List.map ~f:(fun x -> if x = ' ' then "&nbsp;" else String.make 1 x) str in


  let list_to_string mylist = 
    let rec list_help str lst = 
    match lst with 
    | [] -> str
    | hd::tl -> list_help (str ^ hd) tl in
   
    list_help "" mylist in

  list_to_string (to_nbsp (string_to_list str));;
      
      
   let rec get_con_scores (query_seq:string) (seq_list:container list) (count:int) : hit list = 
     
      match seq_list with  
      | [] -> []
      | hd::tl -> (*use this match case to skip over empty sequences*)
               (match hd.sequence with 
		| "" -> get_con_scores query_seq tl (count+1)
		| _ -> 
            
              let obj = match_sequences query_seq hd.sequence in
              let seqscore = S.seq_score obj in
              let myseqs = S.get_seqs obj in
              let item = {myscore = seqscore; sequences = myseqs; comment = hd.comment} in
              item::(get_con_scores query_seq tl (count+1)))

 
   let one_shot_print (s:string) : unit = 
       let output_file =  Sys.argv.(3) in
     let out_chan = open_out_gen
          [Open_append] 0o666 output_file in Printf.fprintf out_chan "%s" s;()
    
     let query_info (comment:string) (max:int) (threshold:int) : unit = 
       let align_type = Sys.argv.(2) in
       let output_file = Sys.argv.(3) in
     let out_chan = open_out_gen
          [Open_append] 0o666 output_file in Printf.fprintf out_chan 
          "<p><strong>Query Sequence Info</strong>: %s</p>\n
           <p><strong>Alignment Type:</strong> %s</p>\n
           <p><strong>Max Hits:</strong> %i <strong>Threshold:</strong> %i<p>" comment align_type max threshold;()
          
    let print_footer () : unit = 
     one_shot_print footer_markup;()
    
    let print_header () : unit = 
      one_shot_print header_markup;()

    (*prints sequences to terminal.*)
   let print_seq (h:hit) : unit = 
       let output_file = Sys.argv.(3) in
     let (a,b) = h.sequences in
        Printf.printf  "---------\n score: %i\n%s - query sequence\n%s - test sequence \n%s\n----------" h.myscore a b h.comment;
        let out_chan = open_out_gen
              [Open_append] 0o666 output_file in Printf.fprintf out_chan "
<p>---------</p>\n 
<p> <span class='score'>score: </span>%i</p>\n
<p class='query'><span> %s - query sequence</span></p> \n 
<p class='test'><span>%s - test sequence</span></p> \n
<p class='comment'><span> %s </span></p> \n" h.myscore (nbsp a) (nbsp b) h.comment;() 

        


    (* mergesort and helpers thereof are adapted from CS51 Lecture*)
  
    (*we are sorting hits by score from greatest to least here.*)
    let rec merge xs ys = 
       match xs, ys with 
       | [], _ -> ys
       | _, [] -> xs
       | x :: xs', y :: ys' -> 
          if x.myscore >= y.myscore then x :: merge xs' ys
           else y :: merge xs ys' 

    let rec split xs ys zs =
      match xs with 
      | [] -> (ys, zs) 
      | x :: xs' -> split xs' zs (x :: ys) 

    let rec mergesort xs = 
      match xs with 
      | [] | [_] -> xs
      | _ -> let (xs1, xs2) = split xs [] [] in
            merge (mergesort xs1) (mergesort xs2) 

    let get_elts (max:int) (lst: 'a list) : 'a list = 
      let rec elts_helper (counter:int) (dec: 'a list) (accum: 'a list) : 'a list = 
        match dec with 
        | [] -> accum
        | hd::tl -> if counter >= max then accum 
                    else (elts_helper (counter+1) tl ( accum @ [hd] ) ) in
      elts_helper 0 lst []

    (*return a list of containers with their scores; then filter, sort, truncate, and print*)
    let list_my_objects (query_sequence:string) (index:container list) (threshold:int) (max_hits:int) (comment:string) : hit list =
           
           print_header();
           query_info comment max_hits threshold;
           let unfiltered = get_con_scores query_sequence index 0 in 
           let unsorted = List.filter ~f:(fun x -> x.myscore > threshold) unfiltered in
           let sorted = mergesort unsorted in
           let truncated = get_elts max_hits sorted in
           let _ = List.iter ~f:(fun x -> print_seq x; ()) truncated in print_footer(); truncated

end
