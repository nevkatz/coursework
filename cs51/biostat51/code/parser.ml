 open Core.Std
open In_channel
open Globals

(*This modules parses existing query and database files for use.*)
module type PARSER =
  sig
 
    val input_ch : string -> in_channel
    val file_to_con : string -> container
    val readLines : in_channel -> string option list
    val fix_x : string list -> container list
    val print_container_list : container list -> unit
    val print_container : container -> unit
    val concat_my_files:  string list -> container list
  end


module SeqCamlParser : PARSER =
struct

  (*declare filename - we may want to have these in an array.*)
  (*we should have a list, or an array, of filenames.*)


  (*function for opening the channel to the file*)
  let input_ch filename = open_in filename

  (*type container = {sequence:string; comment:string}*)
  (*reads one line at a time*)
  let rec readLines f_name  =
    try let line = input_line f_name in line::(readLines f_name)
    with End_of_file -> []


 (*takes a list of strings from a file and turns it into a list of containers*)
 let rec fix_x (x:string list) : container list =
  match x with
   | [] -> []
   | hd1 :: [] -> [{ sequence = hd1 ; comment = ""}]
   | hd1 :: hd2 :: tl -> { sequence = hd1 ; comment = hd2} :: (fix_x tl) 

(*do we need a match statement here?*)
let print_container x =
  (print_string x.sequence; print_string " " ; print_endline x.comment)

(*print the whole list.*)
 let rec print_container_list x =
  match x with
   | [] -> print_string "NULL"
   | hd1 :: [] -> print_container hd1
   | hd1 :: tl -> print_container hd1; (print_container_list tl) 

(*
 *  NEW PARSER FUNCTIONS
 *)

(*takes a filename with one sequence-comment pair, gets contents, and puts it into a container.*)
let file_to_con filename = 
  if Sys.file_exists filename = `Yes then
  let file = In_channel.create filename in
  let seq =  In_channel.input_lines file in
  let to_type = 
     match fix_x seq with
     | [] -> {sequence = ""; comment = ""}
     | h::_ -> h in
  
  (*let sum = List.fold ~init:0 ~f:(fun x y -> x::y) seq in*)
  In_channel.close file; to_type
  (*if file doesn't exist, return none.*)
  else {sequence = "none"; comment=""};;

(*takes a filename with multiple seq-comment pairs, gets contents, puts it into a list.*)
let file_to_con_list filename  = 
  let file = In_channel.create filename in
  let seq =  In_channel.input_lines file in
  let to_type = fix_x seq in
  In_channel.close file; to_type;;

(*takes a list of files and makes a list of ALL sequences within these files.*)
let rec concat_my_files file_list = 
  match file_list with 
  | [] -> []
  | h::t -> file_to_con_list h @ concat_my_files t;;

(*converts list of filenames into list of containers, as long as each file contains only one sequence pair.*)
(*let list_of_files file_list = List.map ~f:(fun x -> list_file x) file_list;;*)

end;;


