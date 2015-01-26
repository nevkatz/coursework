(* 
title: bioinformatics sequencing & conversion tool
seas-usernames: nkatz01, rburgess
*)

open Core.Std
open In_channel
open Globals
open Parser
open Matrix
open Algorithms
open Responder
open Converter


(*
 *   Outline of Modules Types & Modules
 *   -----------------------------------
 *   Module Type      |    Module(s)
 *   UI               |    Biostat
 *                    | 
 *   PARSER           |    SequCamlParser
 *                    | 
 *   MATRIX           |    DNA, Protein
 *                    |
 *   SEQ_ALG          |    align_sequences (functor), Alg_DNA (functor with DNA Matrix passed in)                 |    
 *                    | 
 *   QUERY_RESPONDER  |    Traverse_Sequences (functor)
 *                    | 
 *   CONVERTER        |    SeqCon
 *)


(*might want to keep this universal*)

(*
 *  DNA AND PROTEIN MODULES
 *)

module Alg_DNA = (Sequence_Align (DNA) : SEQ_ALG);;

module Alg_PROTEIN = (Sequence_Align (Protein) : SEQ_ALG);;

(*build the query module using the Alg algorithm with the DNA/RNA argument.*)
module QR_Alg_DNA = (Traverse_Sequences (Alg_DNA) : QUERY_RESPONDER);;

module QR_Alg_PROTEIN = (Traverse_Sequences (Alg_PROTEIN) : QUERY_RESPONDER);;
(*so here is the demo - you pass in a sequence, a threshold, and a max # of hits and it will return some results IN ORDER!.*)


(*this is the main module that interacts with the user and controls other modules.*)
module type UI = 
sig 

  val get_query : unit -> string * string
  
  val parse_args: unit -> unit
end


module Biostat:UI = 
struct

  
  let is_int (str:string) : bool = 
  let nums = "0123456789" in
  let rec helper (count:int) = 
  if count >= String.length str then true else
    let cur_char = String.get str count in
   if String.contains nums cur_char || (cur_char = '-' && count=0) then helper (count+1) else false in
   helper 0 
  
(*get string from a file*)
let get_string_from_file filename = 
 let con = SeqCamlParser.file_to_con filename in
 (con.sequence, con.comment);;
 
(*get file extension*)
  let file_ex str trace =
  if String.length str < trace then "none" else
  let len = String.length str in
   String.sub str ~pos:(len-trace) ~len:trace ;;
 
 let rec get_input prompt condition correction : string = 
 Out_channel.output_string stdout prompt; 
 Out_channel.flush stdout;
 let inp = input_line stdin in
 if condition inp = false then get_input correction condition correction else
 match inp with 
 | None -> ""
 | Some x -> x;;

(*gets a short comment.*)
let get_comment () : string = 
 let max_chars = 50 in
 let mc = string_of_int max_chars in
 let prompt = "Write a comment (" ^ mc ^ " characters or less) and then hit enter.\n"  in
 let condition = (fun y -> match y with 
                  | None -> false 
                  | Some x -> if String.length x > max_chars then false else true) in
 let correction = "Comment should be " ^ mc ^ " characters or less.\n"  in
 let comment = get_input prompt condition correction in comment

(*the main query getter.*)
let rec get_query () : (string * string) = 
  let prompt = "Input a sequence or filename: " in
  Out_channel.output_string stdout prompt;
  Out_channel.flush stdout;
  let query = input_line stdin in

  match query with 
  | None -> Printf.printf "Nothing entered"; get_query();
  | Some x -> if x = "" then let _ = Printf.printf "Nothing entered.\n" in get_query ()
              else if file_ex x 4 = ".seq" then 
              let dir = "./query/" in
              (match get_string_from_file (dir ^ x) with 
              | ("none",_) -> let _ = Printf.printf "File not found. Make sure the file is in the query directory.\n" in get_query ()
              (* tuple of (sequence, comment) *)
              | file_data ->   file_data) 
              (*if an entered sequence - maybe revise comment for better validation*)
              else let comment = get_comment () in
                  (x, comment);;
                  
(*main usage message.*)
let usage () = let seqmode = "\n******MODE 1: Sequence Alignment******\n\n" in
               let arg1a = "arg 1: DNA | RNA | protein\n" in
               let arg2a = "arg 2: local | global | repeats\n" in
               let arg3a = "arg 3: output_filename.html\n\n" in
               let conmode = "******MODE 2: Sequence Conversion*******\n\n" in
               let arg1b = "args 1-3: DNA to RNA | DNA to cDNA | RNA to protein \n" in
               let arg2b = "arg 4: output_filename.seq\n\n" in
               Printf.printf "usage: %s %s %s %s %s %s %s %s" Sys.argv.(0) seqmode arg1a arg2a arg3a conmode arg1b arg2b;
               exit 1
               
let incorrect_input (subunit:string) : unit = 
  Printf.printf "Entered Sequence contains elements other than %s. Run program again.\n" subunit; exit 1;;

(*print sequence to a file*)
   let print_sequence seq ofile : unit = 
     let out_chan2 = open_out ofile in output_string out_chan2 "";
     let out_chan = open_out_gen
          [Open_append] 0o666 ofile in Printf.fprintf out_chan "%s\n" seq;()

let parse_args_convert () : unit = 
   if Array.length Sys.argv <> 5 then usage ();
 let ofile =  Sys.argv.(4) in
 if not (file_ex ofile 4 = ".seq") then usage();
  
 (*for RNA to protein translation, we want the option to wait or to start translating.
 This depends on whether the user wants any of the sequence data prior to the first start codon.*)
 let start_option my_query = 
  let prompt = "Enter (W) to wait for a start (AUG) codon. Enter (S) to start translating right away.\n" in
  let condition = (fun y -> match y with 
                            | None -> false
                            | Some x -> let ux = String.uppercase x in
                              if String.uppercase(ux) = "W" || ux = "S" then true else false) in
                              
  let correction = "Please enter (W) to wait or (S) to start.\n" in
  let opt = get_input prompt condition correction in 
  match opt with 
  | "S" -> SeqCon.ribosome my_query false
  | _ -> SeqCon.ribosome my_query true in
 
 let orig = String.uppercase(Sys.argv.(1)) in
 let dest = String.uppercase(Sys.argv.(3)) in
  let fn =
   match (orig,dest) with
   | ("DNA","RNA") -> (fun x -> SeqCon.dna_rna x)
   | ("RNA","PROTEIN")-> (fun x -> start_option x)
   | ("DNA","CDNA") -> (fun x -> SeqCon.to_cdna x)
   | (_,_) -> usage (); (fun x -> x) in
   
 (*ADD USER VALIDATION BEFORE QUERY - any way to use just one match case?*)
 let (my_query, comment) = get_query () in

 let seq = fn my_query in
 (*i want the user-comment to be on the same line as the final auto-comment so these sequences can be read back in for alignment.*)
 let seq_com = if dest = "PROTEIN" then seq ^ " | " ^ comment else seq ^ "\n" ^ comment in

 (*maybe use match to decide on a function BEFORE the query is taken.*)

  print_sequence seq_com ofile; ()

(*gets user input for sequence alignment.*)
let parse_args_align () : unit =

  if Array.length Sys.argv <> 4 then usage ();
  let output_file =  Sys.argv.(3) in
  if not (file_ex output_file 5 = ".html") then usage();
   let out_chan = open_out output_file in output_string out_chan "";
  (*establish databases*)

  let dir_rna = "./rna_data/" in
  let dir_dna = "./dna_data/" in
  let dir_pro = "./pro_data/" in
  
  let rna_data = ["rna1.seq";"rna2.seq"] in
  let dna_data = ["dna1.seq"; "dna2.seq"] in
  let protein_data = ["protein1.seq"; "protein2.seq"] in
  
  (*add directory path to each filename*)
  let add_dir lst dir = List.map ~f:(fun x -> dir ^ x) lst in
  
  (*alter databases accordingly*)
  let rna_data = add_dir rna_data dir_rna in
  let dna_data = add_dir dna_data dir_dna in
  let protein_data = add_dir protein_data dir_pro in
 
  
  (*query validation*)
  let validate_query (query:string) (str:string) : bool = 
  let rec helper (count:int) = 
  if count >= String.length query then true else
  let cur_char = Char.uppercase (String.get query count) in
  if String.contains str cur_char  then helper (count+1) else false in
  helper 0 in
  
  (*entered strings must only have elements from the pertinent string from the three below.*)
  let amino_acids = "ARGNDCQEGHILKMFPSTWYV" in
  let dna_bases = "ACTG" in
  let rna_bases = "ACUG" in
 
  (*get a bottom limit to the scores that get returned.*)
  let get_threshold () = 
  let prompt = "Enter the lowest possible sequence score. Sequences with a score lower than this will not be returned.\n" in
  let condition = (fun y -> match y with 
                           | None -> false
                           | Some x -> if not (is_int x) then false else
                                       if Int.abs (int_of_string x) < 10000 then true else false) in
  let correction = "Enter an integer between -10000 and 1000.\n" in
  int_of_string (get_input prompt condition correction) in
  
  let get_max () = 
  let prompt = "Enter the maximum number of hits you would like returned in this search: " in
  let condition = (fun y -> match y with 
                           | None -> false
                           | Some x -> if not (is_int x) then false else
                                       if int_of_string x < 10000 && int_of_string x > 0 then true else false) in
  let correction = "Enter an integer between 1 and 1000.\n" in
  int_of_string (get_input prompt condition correction) in
  
  let get_align_info (basetype:string) (subunit:string) (database:string list) fn : unit = 
  let threshold = get_threshold() in
  let max = get_max () in
   let (query, comment) = get_query () in
    if validate_query query basetype = false then incorrect_input subunit else 
    let index = SeqCamlParser.concat_my_files database in
    fn query index threshold max comment; () in 
   
  (*decide based on DNA, RNA, or protein*)
  match String.uppercase (Sys.argv.(1)) with
  (*x = query, y = index, t = threshold, m = max, c = comment*)
  | "DNA" ->  let _ =   get_align_info dna_bases "nucleotides" dna_data
                      (fun x y t m c-> let _ =  QR_Alg_DNA.list_my_objects x y t m c in () ) in ()
         
  | "RNA" -> let _ = get_align_info rna_bases "nucleotides" rna_data
                      (fun x y t m c-> let _ =  QR_Alg_DNA.list_my_objects x y t m c in () ) in ()       
             
  | "PROTEIN"->  let _ = get_align_info amino_acids "amino acids" protein_data
                      (fun x y t m c -> let _ =  QR_Alg_PROTEIN.list_my_objects x y t m c in () ) in ()
                     
                
  | _ ->  Printf.printf "str\n";usage (); ()

(*client validation & decides whether to move on*)
 let parse_args () : unit = 
 
   if Array.length Sys.argv < 4 then usage ();
  let mode = String.uppercase (Sys.argv.(2)) in
  let args = ["LOCAL";"REPEATS";"GLOBAL";"TO"] in
  if not (List.mem args mode) then usage ();
   match mode with 
   | "TO" ->  parse_args_convert ()
   | _ ->  parse_args_align ()
   
end

(*main function.*)
let run () : unit =

  Biostat.parse_args ();();;

run () ;;

