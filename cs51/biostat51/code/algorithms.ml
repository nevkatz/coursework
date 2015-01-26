open Core.Std
open Matrix


(* SEQUENCE ALIGNMENT ALGORITHM MODULE TYPE*)
module type SEQ_ALG = 
sig

  type aligned_sequence 

 (* based on two x-y coordinates, this will return the node in the base matrix.*)
  val getNode : int -> int -> int array array -> int 

 (*three algorithms for aligning DNA sequences.*)
 val align_local : string -> string -> aligned_sequence
 
 val align_repeats : string -> string -> aligned_sequence
 
 (* the main function that aligns two sequences using the NW algorithm.*)
 val align_global : string -> string -> aligned_sequence (*(int * (string * string)) (* * (int * int) array array)*)*)

 (*initializes a score matrix.  Each node in the matrix starts with a tuple of (0,0).*)
 (*the tuple corresponds to (value,path) wherein value is the score and path tells you the
   neighboring box (upleft = 0, up = 1, or left = -1) from which the score was derived*)
 val init_matrix : string -> string -> (int * int) array array

 (*gives you the maximum of three scores; 
   returns 1) the highest score and 2) the box from which it was derived.*)
 val max_trace : int -> int -> int -> bool -> (int * int) 

 (*adds a (score, path) tuple to a node in the matrix.*)
 val set_score_path : (int*int) -> int -> int -> (int*int) array array -> unit

 (*gets the score for a match between two chars using the scoring matrix.*)
 val getScore : char -> char -> int array array -> int

 val seq_score : aligned_sequence -> int

 val get_seqs : aligned_sequence -> ( string * string )

end


(*
 *NEEDLEMAN-WUNSCH & SMITH-WATERMAN ALGORITHM FUNCTOR
 *)

module Sequence_Align (M:MATRIX) : SEQ_ALG = 
struct

(*the result of the algorithm.*)
type aligned_sequence = { score: int; seqs: (string * string) }

(*debug: let get_width = M.get_width

let get_size = M.get_size*)

(*get value in a matrix based on x and y coordinates*)
let getNode (x:int) (y:int) (arr: int array array) : int = 
  let row = Array.get arr y in
    Array.get row x;;

(*get value from a tuple in a matrix based on x and y coordinates*)
let getValue (x:int) (y:int) (arr: (int*int) array array) : int  = 
  let row = Array.get arr y in
  let (value, _) = Array.get row x in value;;

(*get path from a tuple in a matrix based on x and y coordinates*)
let getPath (x:int) (y:int) (arr: (int*int) array array) : int = 
  let row = Array.get arr y in
  let (_, path) = Array.get row x in path;;

(*get score for a match between two chars from scoring matrix - USES THE MATRIX MODULE*)
let getScore (c1:char) (c2:char) (arr: int array array) : int  =
  let x = M.index_of c1 in
  let y = M.index_of c2 in
  getNode x y arr;;

(*set a node in the sequence matrix!*)
let set_score_path (value:int*int) 
                 (x:int)
                 (y:int) 
               (arr:(int*int) array array) : unit =  

  let row = Array.get arr y in
  let _ = Array.set row x value in
  ();;			   

  (*In the local and repeat algorithms we switch strings at the start based on size.
    This function switch strings back at the end if they have been switched.*)
 let check_order in1 in2 out1 out2 = 
 if String.length in2 > String.length in1 then (out2, out1) else (out1, out2) 
 
(* get max of three ints with traceback path*)

(*0 = up-left, -1 = left, 1 = up*)
let max_trace (upleft:int) (left:int) (up:int) (local:bool) : int * int = 
  let a = max upleft up in 
  let max_score = max left a in
  (*for local alignment, we don't allow for negative scores.*)
  let top = if max_score < 0 && local then 0 else max_score in
 
  match (top=upleft,top=left,top=up) with 
  | (true,_,_) -> (top, 0)
  | (_,true,_) -> (top, -1)
  | (_,_,true) -> (top, 1)
  (*Specific case for local alignment; this means we're starting the path over.*)
  | (_,_,_) -> (top, 2) 			    
					      

(*initialize scoring matrix*)
let init_matrix (s1:string) (s2:string) : (int*int) array array = 
  let h = String.length s1 in
  let w = String.length s2 in
  Array.make_matrix ~dimx:w ~dimy:h (0,0);;
  
(*debug: prints out a 2D matrix*)
(*
let print_matrix  (matrix:(int*int) array array) : unit = 
  (*get number of arrays in 2D array*)
  let mat_len = Array.length matrix in
  
  let rec print_helper (pos:(int*int)) : unit = 
  (*get position*)
  let (x,y) = pos in
 
  let cur_array = Array.get matrix y in
  let len = Array.length cur_array in
 
    let node = Array.get cur_array x in
    let (value,path) = node in
    Printf.printf "| (%i,%i) " value path;
    if value < 10 && value >= 0 then Printf.printf " ";
    if path < 10 && path >= 0 then Printf.printf " ";
    if y >= mat_len-1 && x >= len-1 then let _ = Printf.printf "\n\n" in () else
    if x >= len-1 then let _ = Printf.printf "\n" in

   print_helper (0,(y+1)) else print_helper ((x+1),y) in
   print_helper (0,0)   *)
   
   (* debug: 
   let decide_print mat = 
   if get_size mat < 200 && get_width mat < 12 then print_matrix mat
   else Printf.printf "Matrix was too large to be printed.\n"; () *)
(*
 *
 *    Looking for repeat matches in a sequence
 *
 *)
 
let align_repeats (seq1:string) (seq2:string) : aligned_sequence  = 

(*s1 needs to be the longer string*)
let (s1, s2) = if String.length seq1 >= String.length seq2 
then (seq1, seq2) else (seq2, seq1) in

(*returns list of (int,index) tuples from a given column*)
let list_of arr col : (int*int) list = 
  let rec helper count = 
    if count = Array.length arr then [] else
    let cur_arr = Array.get arr count in
    let num = Array.get cur_arr col in
    let (x,_) = num in
    (x,count)::(helper (count+1))
  in helper 0 in
  
  (*max of tuple list*)
  let max_of_list lst = List.fold_left ~f:(fun (x1,x2) (y1,y2) ->  if x1 > y1 then (x1,x2) else (y1,y2)) ~init:(0,0) lst in

 (*max of a column in a tuple matrix with its index instead of its path*)
  let max_of_col arr index = max_of_list (list_of arr index) in

 
  (*0 = up-left, -1 = left, 1 = up, -2 top*)
let max_path (upleft:int) (left:int) (up:int) (top:int) : int * int = 
 let max_of_list lst = List.fold_left ~f:(fun x y -> max x y) ~init:0 lst in
 let mxn = max_of_list [upleft;left;up;top] in
  match (mxn=upleft,mxn=left,mxn=up,mxn=top) with 
  | (true,_,_,_) -> (mxn, 0)
  | (_,true,_,_) -> (mxn, -1)
  | (_,_,true,_) -> (mxn, 1)
  (*effectively marks the end of the path*)
  | (_,_,_,_) -> (mxn, -2)  in
  (*Specific case for local alignment; this means we're starting the path over.*)

let path_from_top (prevcol:int) (thresh:int) (mat:(int*int) array array) : int * int = 
  (*get the max value from the previous column with its index in a tuple.*)
  (*if in upper left, then return a 0,0*)
   if prevcol < 0 then (0,0) else
  (*else, compare max of prev column to top of prev column*)
  let max_prev_col = max_of_col mat prevcol in
  let (value,index) = max_prev_col in 
  let diff = value - thresh in
  
  (*get the previous topmost value.*)
  let prev_top = getValue prevcol 0 mat in
  if diff > prev_top then (diff, index) else (prev_top, 0) in
  
  

  (*gap penalty*)
  let pen = -8 in 

  (*highest indices or horiz,vert respectively*)
  let max1 = (String.length s1) in
  (*extra row for the top row.*)
  let max2 = (String.length s2) in 

  let sa = s1 ^ "_" in
  let sb = s2 ^ "_" in
  (*initialize a zero matrix with an extra row at top, and extra column on right.*)
  let mat = init_matrix sa sb in 

  (*recursive helper function for getting scores*)
  let rec calc_scores (cx: int) (cy:int) : (int*int)   = 
    
    
    
    (*if at top, we don't do a match test - we look at prev column and prev top number.*)
  
    if cy = 0 then let thr = 20 in
    let score = path_from_top (cx-1) thr mat in
                               (*add top score to matrix*)
                               let _ = set_score_path score cx cy mat in
                               (*return here or go down the column*)
                               if cx >= max1 then score else 
                               calc_scores cx (cy+1)                    
    else
    
    (*get a score based on the current chars*)
    let c1 = String.get s1 cx in
    let c2 = String.get s2 (cy-1) in
    
    (*score from base matrix based on current char matching*)
    let base_score = getScore c1 c2 M.ref_matrix in 

    (*add some conditions where string is significantly shorter*)
    
    (*determine score for upleft*)
    let upleft = 
        match (cx,cy) with
	    | (0,_) -> 0 (*ok to use cy, because it is top left*)
        | (_,_) -> getValue (cx-1) (cy-1) mat in

    (*if at top row, penalty * pos in row; else, get score up top*)
    let up = getValue cx (cy-1) mat in

    (*if in left column, penalty * pos in col; else, get score on left*)
    let left = if cx = 0 then 0
               else getValue (cx-1) cy mat in
               
    let topmost = getValue cx 0 mat in

    (*get maximum of the three options - set local to false *)
    let score = max_path (base_score + upleft) (left + pen) (up + pen) (topmost) in
    
    (*input the score into the matrix*)
    set_score_path score cx cy mat; 

    (*if y-count less than col length, go to next char in col... *)
    if cy < max2 then calc_scores cx (cy+1) else

    
    (*else go to next col.  
      if x-count less than # cols, start at next col. values get returned at top-right.*)
      calc_scores (cx+1) 0 in

    (*run the calc_scores function*)
    let (final_score, _) = calc_scores 0 0 in

    (* traceback *)

 let rec t_helper (align1:string) (align2:string) (rem1:int) (rem2:int) : (string * string) =
     (*so it looks like it's not jumping back....*)
     
     let char1 = if rem1 >= 0 && rem1 < max1 then String.make 1 (String.get s1 (rem1)) else "-" in

     let char2 = if rem2 > 0 then String.make 1 (String.get s2 (rem2-1)) else "-" in

     (*stops when it reaches upper left*)
     if rem1 < 0 then check_order seq1 seq2 align1 align2 else

     let path = getPath rem1 rem2 mat in
   
         
     (*if starting at final score, then don't add to the strings and just follow the path*)
     if rem1 = max1 then t_helper (align1) (align2) (rem1-1) path
     (*if at the top somewhere else, go by these rules...*)
     else if rem2 = 0 then
     (match path with
     | 0 -> t_helper  (char1 ^ align1) (char2 ^ align2) (rem1-1) 0
     | v -> t_helper  (char1 ^ align1) ("." ^ align2) (rem1-1) v) 
     
     else
     
 
     (match path with
     | 0 ->  t_helper (char1 ^ align1) (char2 ^ align2) (rem1-1) (rem2-1) (*get node from map*)
            
     | 1 -> t_helper (char1 ^ align1) (char2 ^ align2) (rem1) (rem2-1)  
     
     | -1 ->  t_helper (char1 ^ align1) ("-" ^ align2) (rem1-1) (rem2)
     
     | _ -> t_helper (align1) (align2) (rem1) 0) in (*should I be appending here?*)
    
  
     (*set starting point to upper right in that extra column*)
     {score = final_score; seqs = t_helper "" "" max1 0} 



(*Smith-Waterman Algorithm - for local alignment (finding one sequence in another sequence) *)
let align_local (seq1:string) (seq2:string) : aligned_sequence  = 

(*s1 needs to be the longer string*)
let (s1, s2) = if String.length seq1 >= String.length seq2 
then (seq1, seq2) else (seq2, seq1) in
  
  (*gap penalty*)
  let pen = -8 in 

  (*highest indices or horiz,vert respectively*)
  let max1 = (String.length s1) - 1 in
  let max2 = (String.length s2) - 1 in

  (*initialize a zero matrix*)
  let mat = init_matrix s1 s2 in 
  
 let max_1d (arr:(int*int) array) : ((int*int) * int) = 
   let rec max_helper  (ind:int) (prev:((int*int)*int)) : ((int * int) *int) = 
  
   if ind >= (Array.length arr) then prev else
   (*get the latest number using the index*)
   let cur_tuple = Array.get arr ind in
   let (cur,_) = cur_tuple  in
   (*get the previous max number and its location in the array*)
   let ((p_num,_), _) = prev in
  
   let new_max = if cur >= p_num then (cur_tuple,ind) else prev in
   max_helper (ind+1) new_max in
   max_helper  0 ((0,0),0) in

(*returns the (val,path) pair with the highest value from the matrix with its (x,y) location*)
  let max_2d (arr:(int*int) array array) : ((int * int) * (int * int)) = 
    let rec max_helper (cur_y:int) (prev:((int*int)*(int*int))) : ((int*int) * (int*int)) = 
    if cur_y >= (Array.length arr) then prev else
    (*get current array*)
    let cur = Array.get arr cur_y in
    let ((prev_val,_), (_,_)) = prev in
    let (cur_tuple, cur_x) = max_1d cur in
    let (cur_val,_) = cur_tuple in
    let new_max = 
      if cur_val >= prev_val then (cur_tuple, (cur_x, cur_y))
      else prev in
    max_helper (cur_y+1) new_max in
    max_helper 0 ((0,0),(0,0)) in

  (*recursive helper function for getting (max_score,path) (x,y)*)
  let rec calc_scores (cx: int) (cy:int) : ((int*int)*(int*int))   = 
    
    (*get a score based on the current chars*)
    let c1 = String.get s1 cx in
    let c2 = String.get s2 cy in
    
    (*score from base matrix based on current char matching*)
    let base_score = getScore c1 c2 M.ref_matrix in 

    (*determine score based on coordinates*)
    let upleft = 
        match (cx,cy) with
	    | (0,_) -> 0 (*ok to use cy, because it is top left*)
        | (_,0) -> 0
        | (_,_) -> getValue (cx-1) (cy-1) mat in

    (*if at top row, penalty * pos in row; else, get score up top*)
    let up = if cy = 0 then 0
             else getValue cx (cy-1) mat in

    (*if in left column, penalty * pos in col; else, get score on left*)
    let left = if cx = 0 then 0
               else getValue (cx-1) cy mat in

    (*get maximum of the three options - set local to true*)
    let score = max_trace (base_score + upleft) (left + pen) (up + pen) true in
 
    (*input the score into the matrix*)
    set_score_path score cx cy mat; 

    (*if x-count less than row length, go to next char in row... *)
    if cx  < max1 then calc_scores (cx+1) cy else

    
    (*else go to next row.  
      if y-count less than # rows, start at next row; else return value*)
    if cy < max2 then calc_scores 0 (cy+1) else max_2d mat 
   
     in 
    (*end of calc_scores*)
   
    let ((max_score, _),(max_score_x, max_score_y)) = calc_scores 0 0 in

    (* traceback *)


 
 let rec t_helper (align1:string) (align2:string) (rem1:int) (rem2:int) : (string * string) =
     
     let char1 = if rem1 >= 0 then String.make 1 (String.get s1 rem1) else "-" in

     let char2 = if rem2 >= 0 then String.make 1 (String.get s2 rem2) else "-" in
     
    
   
     match (rem1 < 0, rem2 < 0) with
     | (true, true) ->   check_order seq1 seq2 align1 align2

     | (false, true ) ->
                         t_helper (char1 ^ align1) (" " ^ align2) (rem1-1) (rem2) 
      | (true, false) ->  t_helper (" " ^ align1) (char2 ^ align2) (rem1) (rem2-1) 

      (*false, false*)
     | (_,_) ->  

     let path = getPath rem1 rem2 mat in
    
     (match path with
     | 0 -> t_helper (char1 ^ align1) (char2 ^ align2) (rem1-1) (rem2-1) (*get node from map*)
     | 1 ->  t_helper ("-" ^ align1) (char2 ^ align2) (rem1) (rem2-1) 
     | -1 ->   t_helper (char1 ^ align1) ("-" ^ align2) (rem1-1) (rem2)
      (*if at a path-start, end the traceback*)
     | _ ->            let head = String.sub s1 ~pos:0 ~len:(rem1-1) in
                      
                       
                       if String.length head > 0 then
                       let space = String.make (String.length head) ' ' in
                       (*here we are switching the strings back if needed so query = query and test = test in output*)
                       check_order seq1 seq2 (head ^ align1) (space ^ align2)
                       else (check_order seq1 seq2 align1 align2))
     
     in
      let get_tail s rem = 
      let len = String.length s in
      let dist = len - rem in
      String.sub s ~pos:rem ~len:dist in
      
      let tail1 = get_tail s1 max_score_x in
      let tail2 = get_tail s2 max_score_y in
      let t1 = if String.length s1 > String.length s2 then tail1 else String.make (String.length tail2) ' ' in
      let t2 = if String.length s2 > String.length s1 then tail2 else String.make (String.length tail1) ' ' in

     {score = max_score; seqs = t_helper t1 t2 max_score_x max_score_y} 


(*Needleman-Wunsch*)
let align_global (s1:string) (s2:string) : aligned_sequence  = 

  (*gap penalty*)
  let pen = -8 in 

  (*highest indices or horiz,vert respectively*)
  let max1 = (String.length s1) - 1 in
  let max2 = (String.length s2) - 1 in

  (*initialize a zero matrix*)
  let mat = init_matrix s1 s2 in 

  (*recursive helper function for getting scores*)
  let rec calc_scores (cx: int) (cy:int) : (int*int)   = 
    
    (*get a score based on the current chars*)
    let c1 = String.get s1 cx in
    let c2 = String.get s2 cy in
    
    (*score from base matrix based on current char matching*)
    let base_score = getScore c1 c2 M.ref_matrix in 

    (*add some conditions where string is significantly shorter*)

    (*determine score for upleft*)
    let upleft = 
        match (cx,cy) with
        | (0,0) -> 0
	| (0,_) -> pen*cy (*ok to use cy, because it is top left*)
        | (_,0) -> pen*cx
        | (_,_) -> getValue (cx-1) (cy-1) mat in

    (*if at top row, penalty * pos in row; else, get score up top*)
    let up = if cy = 0 then pen*(cx+1) 
             else getValue cx (cy-1) mat in

    (*if in left column, penalty * pos in col; else, get score on left*)
    let left = if cx = 0 then pen*(cy+1) 
               else getValue (cx-1) cy mat in

    (*get maximum of the three options - set local to false *)
    let score = max_trace (base_score + upleft) (left + pen) (up + pen) false in
    
    (*input the score into the matrix*)
    set_score_path score cx cy mat; 

    (*if x-count less than row length, go to next char in row... *)
    if cx  < max1 then calc_scores (cx+1) cy else

    
    (*else go to next row.  
      if y-count less than # rows, start at next row; else print matrix and return value*)
    if cy < max2 then calc_scores 0 (cy+1) else score in

    let (final_score, _) = calc_scores 0 0 in

    (* traceback *)

 let rec t_helper (align1:string) (align2:string) (rem1:int) (rem2:int) : (string * string) =
     
     let char1 = if rem1 >= 0 then String.make 1 (String.get s1 rem1) else "-" in

     let char2 = if rem2 >= 0 then String.make 1 (String.get s2 rem2) else "-" in


     match (rem1 < 0, rem2 < 0) with
     | (true, true) ->  (align1,align2) 
     | (false, true ) -> t_helper (char1 ^ align1) ("-" ^ align2) (rem1-1) (rem2) 
     | (true, false) -> t_helper ("-" ^ align1) (char2 ^ align2) (rem1) (rem2-1)

     (*if both have chars left*)
     | (_,_) ->  

     let path = getPath rem1 rem2 mat in
 
     (match path with
     | 0 -> t_helper (char1 ^ align1) (char2 ^ align2) (rem1-1) (rem2-1) (*get node from map*)
     | 1 -> t_helper ("-" ^ align1) (char2 ^ align2) (rem1) (rem2-1)  
     | _ -> t_helper (char1 ^ align1) ("-" ^ align2) (rem1-1) (rem2)) in
  
     {score = final_score; seqs = t_helper "" "" max1 max2} 

let get_seqs (a:aligned_sequence) : (string * string) = 
  a.seqs

let seq_score (a:aligned_sequence) : int = a.score

end
