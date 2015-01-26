
module type MATRIX = 
sig
  exception IndexMismatch

 (* this matrix tells you the score obtained from comparing two given chars.*)
  val ref_matrix : int array array 

 (* this will give you the index of a given char in the base matrix*)
  val index_of : char -> int
  
 (*DEBUG:
  val get_size : 'a array array -> int
  
  val get_width: 'a array array -> int*)

end


module DNA : MATRIX = 
struct
  (*scoring matrix - can be used for DNA or RNA*)


  exception IndexMismatch
  (*                    a   c   g   t/u   *)

  let a: int array = [| 2; -1; -1; -1 |];;

  let c: int array = [| -1; 2; -1; -1 |];;

  let g: int array = [| -1; -1; 2; -1 |];;

  let t: int array = [| -1; -1; -1; 2 |];;

  (* Condensed version: 
     A  C  G  T
  A  2 -1 -1 -1
  C -1  2 -1 -1
  G -1 -1  2 -1
  T -1 -1 -1  2
  *)

  let ref_matrix = [|a;c;g;t|];;

  (*get index of a character in our 1D array*)
  let index_of (mychar:char) : int =
   match mychar with
     | 'a' | 'A' -> 0
     | 'c' | 'C' -> 1
     | 'g' | 'G' -> 2
     | 't' | 'T' | 'u' | 'U' -> 3
     | _ -> Printf.printf "problem char: %c\n" mychar; raise IndexMismatch
     
  (* DEBUG: let get_width (arr:'a array array) : int = 
    let sam_arr = Array.get arr 0 in
    Array.length sam_arr;;


  let get_size (arr:'a array array) : int = 
   
    let height = Array.length arr in
    let width = get_width arr in
    let area = width*height in
    area;;*)
    
end


(*
 *  AMINO ACID SEQUENCE MATRIX
 *) 

(* the 2D array below is an exact replica of the blosum50 matrix, coded up in ocaml.
   The one-letter abbreviation for each amino acid is below, with one letter and 3 letter codes.*)
(*
Some Sample Amino Acids With abbreviations (hydrophobic)

amino acid    
glycine 	    Gly	G
alanine 	    Ala	A
valine 	        Val	V
leucine 	    Leu	L
isoleucine 	    Ile	I
methionine 	    Met	M
phenylalanine 	Phe	F
tryptophan 	    Trp	W
proline 	    Pro	P

Polar (hydrophilic)
serine 	        Ser	S
threonine 	Thr	T
cysteine 	Cys	C
tyrosine 	Tyr	Y
asparagine 	Asn	N
glutamine 	Gln	Q

Notice that matches between similar amino acids give you a reasonable score, and matches between identical amino acids give you a more positive score. matches between unlike amino acids will yield a negative score.*)
 
module Protein : MATRIX = 
struct

   exception IndexMismatch

                         (*A   R   N   D   C   Q   E   G   H   I   L   K   M   F   P   S   T   W   Y   V *)
    let a: int array = [|  5; -2; -1; -2; -1; -1; -1;  0; -2; -1; -2; -1; -1; -3; -1;  1;  0; -3; -2;  0|]
    let r: int array = [| -2;  7; -1; -2; -4;  1;  0;  -3; 0; -4; -3;  3; -2; -3; -3; -1; -1; -3; -1; -3|]
    let n: int array = [| -1; -1;  7;  2; -2;  0;  0;  0;  1; -3; -4;  0; -2; -4; -2;  1;  0; -4; -2; -3|]
    let d: int array = [| -2; -2;  2;  8; -4;  0;  2; -1; -1; -4; -4; -1; -4; -5; -1;  0; -1; -5; -3; -4|]
    let c: int array = [| -1; -4; -2; -4; 13; -3; -3; -3; -3; -2; -2; -3; -2; -2; -4; -1; -1; -5; -3; -1|]
    let q: int array = [| -1;  1;  0;  0; -3;  7;  2; -2;  1; -3; -2;  2;  0; -4; -1;  0; -1; -1; -1; -3|]
    let e: int array = [| -1;  0;  0;  2; -3;  2;  6; -3;  0; -4; -3;  1; -2; -3; -1; -1; -1; -3; -2; -3|]
    let g: int array = [|  0; -3;  0; -1; -3; -2; -3;  8; -2; -4; -4; -2; -3; -4; -2;  0; -2; -3; -3; -4|]
    let h: int array = [| -2;  0;  1; -1; -3;  1;  0; -2; 10; -4; -3;  0; -1; -1; -2; -1; -2; -3;  2; -4|]
    let i: int array = [| -1; -4; -3; -4; -2; -3; -4; -4; -4;  5;  2; -3;  2;  0; -3; -3; -1; -3; -1;  4|]
    let l: int array = [| -2; -3; -4; -4; -2; -2; -3; -4; -3;  2;  5; -3;  3;  1; -4; -3; -1; -2; -1;  1|]
    let k: int array = [| -1;  3;  0; -1; -3;  2;  1; -2;  0; -3; -3;  6; -2; -4; -1;  0; -1; -3; -2; -3|]
    let m: int array = [| -1; -2; -2; -4; -2;  0; -2; -3; -1;  2;  3; -2;  7;  0; -3; -2; -1; -1;  0;  1|]
    let f: int array = [| -3; -3; -4; -5; -2; -4; -3; -4; -1;  0;  1; -4;  0;  8; -4; -3; -2;  1;  4; -1|]
    let p: int array = [| -1; -3; -2; -1; -4; -1; -1; -2; -2; -3; -4; -1; -3; -4; 10; -1; -1; -4; -3; -3|]
    let s: int array = [|  1; -1;  1;  0; -1;  0; -1;  0; -1; -3; -3;  0; -2; -3; -1;  5;  2; -4; -2; -2|]
    let t: int array = [|  0; -1;  0; -1; -1; -1; -1; -2; -2; -1; -1; -1; -1; -2; -1;  2;  5; -3; -2;  0|]
    let w: int array = [| -3; -3; -4; -5; -5; -1; -3; -3; -3; -3; -2; -3; -1;  1; -4; -4; -4; 15;  2; -3|]
    let y: int array = [| -2; -1; -2; -3; -3; -1; -2; -3;  2; -1; -1; -2;  0;  4; -3; -2; -2;  2;  8; -1|]
    let v: int array = [|  0; -3; -3; -4; -1; -3; -3; -4; -4;  4;  1; -3;  1; -1; -3; -2;  0; -3; -1;  5|]

                     (*0; 1; 2; 3; 4; 5; 6; 7; 8; 9; 10;11;12;13;14;15;16;17;18;19*)
    let ref_matrix = [|a; r; n; d; c; q; e; g; h; i; l; k; m; f; p; s; t; w; y; v|];;

    let index_of (mychar:char) : int = 
    match mychar with
     | 'a' | 'A' -> 0
     | 'r' | 'R' -> 1
     | 'n' | 'N' -> 2
     | 'd' | 'D' -> 3

     | 'c' | 'C' -> 4
     | 'q' | 'Q' -> 5
     | 'e' | 'E' -> 6
     | 'g' | 'G' -> 7

     | 'h' | 'H' -> 8
     | 'i' | 'I' -> 9
     | 'l' | 'L' -> 10
     | 'k' | 'K' -> 11

     | 'm' | 'M' -> 12
     | 'f' | 'F' -> 13
     | 'p' | 'P' -> 14
     | 's' | 'S' -> 15

     | 't' | 'T' -> 16
     | 'w' | 'W' -> 17
     | 'y' | 'Y' -> 18
     | 'v' | 'V' -> 19
   
     |  _ -> raise IndexMismatch
     
 (* DEBUG: let get_width (arr:'a array array) : int = 
    let sam_arr = Array.get arr 0 in
    Array.length sam_arr;;


  let get_size (arr:'a array array) : int = 
   
    let height = Array.length arr in
    let width = get_width arr in
    let area = width*height in
    area;;*)
    
    
end
