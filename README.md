# Simple-regular-expression-library
A simple library providing access to parsing strings using regular expressions. 
Additionally, you can perform some operations with deterministic state machines and syntax trees, including building your own.
===========================================================
The syntax for this regular expression language is:

 # Operation "or": r1 | r2 (metacharacter '|').
	 
 # Concatenation operation: r1r2

 # Operation "positive closure": r + (metacharacter '+') - repeating the operand 1 or more times

 # Operation "optional part": r? (metacharacter '?')

 # "Any character" operation:. (metacharacter '.') - any ascii character. 
   (branching on a specific symbol has a higher priority, branching on '.' is carried out only if
   there are no explicit branches on the current symbol). 

	NOTE: if it is possible to replace a given
  	      metacharacter with a combination of others, then it is better to do this, in this case both the analysis
              of the string and the construction of the automaton will be faster.

 # Operation "repeat expression in range": r {x, y} (metacharacter {x, y}) - borders can be omitted. 
   If the right boundary is omitted {, y} is interpreted as {0, y}, if the left {x,} is interpreted as {x, inf},
   both boundaries can be omitted (an analogue of the Kleene closure is obtained). The construct r {0,0} is allowed,
   but it is interpreted simply as an empty substring, without affecting the regular expression. 

	NOTE:   it is worth trying
   		not to use large finite ranges (it does not affect infinite ranges), because this increases the size of the automaton and the speed
   		of its construction up to y times (worst case if a large range is applied to the expression where there is'.'), also with too large
   		values ​​of the range (usually more than 10, but depending on the situation), restoring the regular expression using the k-path method becomes
   		very memory-intensive, and sometimes even impossible.

 # Named capture group operation: (<name> r) (metacharacter (<name>)) -
   where name is the name of the capture group. Allows you to save the value that was obtained by parsing the corresponding
   section of the string in these brackets. The group name can only consist of letters, and after it there must be some expression,
   and not immediately the closing parenthesis. The name cannot be duplicated within the regular expression. 

	NOTE:   at the moment
   		it does not always work correctly if there is a '.' Inside the capture group. with an unspecified number of occurrences,
   		for example ".?" or ". {2,20}" or ". +" and (less often) if at the very end of the capture group there is any expression
   		with an undefined number of occurrences and is not included in the maximum number of times, in these cases it can grab an extra one)
   		will be fixed in future versions ... If you avoid uncertainty about the number of '.' and uncertainty at the end, it works stably.
   		Formally, although there can be only one capturing group with a given name in a regular
   		expression, the operations (<name> r) +, (<name> r) ?, (<name> r) {x, y} are not prohibited, but in the group
   		capture will write the value that was obtained from the LAST entry of it into the string.

 # Operation "expression from a named capturing group": <name> (metacharacter <name>) - when parsing a string,
   it is replaced with the value that is stored in the capturing group with the corresponding name.At the stage
   of building a regular expression, it throws an exception if a capturing group with the same name is not in the regular expression,
   or it obviously comes after. When parsing a string, it throws an exception if there is a link to a capturing group that
   has not been initialized, for example, in a regular expression:
   "(<a> a)? <a>" when parsing strings ("aa") - everything is fine, the string fits, the string "a" - everything is fine - false,
   the group is initialized, but the string does not match the pattern, when the string " b "- an exception will be thrown,
   because the capturing group has not been initialized, it is optional and in this case 'a' is omitted, however, reaching
   the link to the group, an exception will be thrown. Also try to avoid ambiguities
   of this kind (<a> a) | b or (<a> a) | (<b> b), try to keep the capture group unambiguous.
   And situations like (<a> a)? replace with (<a> a?), in which case everything will be correct.

   	NOTE:   There are still errors, and for example, when parsing by regular expression
   		"(<n> abc) (abk | <n>)" string "abcabc" - the result will be negative, while parsing the same string,
   		by regular expression "(<n> abc) (kbk | <n>) "- will be positive. The problem is that the priority of a jump by a specific
   		character is higher than by a link from a capture group and in "abc" there will be a jump -> a-> b and an attempt to jump to 'c', but 'b' leads only to -> k.
   		Since it is not obvious which branch is more important, by character or substring, you should avoid this kind
  		of ambiguity if you are not sure that such a situation will not arise. In future versions, this problem will be solved,
   		but this will require "rebuilding" the automaton right during the parsing of the string, which will greatly affect
   		the performance of using this operation even in cases without ambiguity. (maybe a separate function will be added, more secure, but slower).

 # The "escaping" operation: & - converts the following metacharacter (a regular character too) into a regular alphabet character. It takes precedence over parentheses.

!!!   Please note that if the syntax is violated in the following operations:
      (<name> r), <name>, r {x, y}, for example, if the name is not only letters, or an extra comma, or x is not a number,
       they will be interpreted as a set of ordinary characters.

 # The "empty string" metacharacter: `- allows you to explicitly separate metacharacters and subexpressions from each other,
   without affecting the result of string parsing in any way. 

	NOTE:   not supported at this time, you can apply {0,0}
   		to any character in the alphabet as a replacement, for example "e {0,0}"

 # Operation "closure of Kleene": r * (metacharacter '*') - repetition of an expression any number
   from 0 to infinity times. 

	NOTE: Not supported yet. As a replacement, you can use "r? +", "R +?", "R {0,}"

 # Operation "any of ...": [r1r2r3r4..rN] (metacharacter '[]') 
	
	NOTE: not yet supported, as a replacement you can use "r1 | r2 | r3 ... | rN"

!!!   The use of operator brackets (r) to change the priority of an operation is supported.
      The parentheses must always go in pairs, the number of open = the number of closed.

        NOTE: that if immediately after the parentheses there is an operation "expression from a named capturing group", (<name> .....),
              then this will be interpreted as a "named" capturing group to separate the "expression" from the parentheses, use the "empty line character".

=============================
	
# Exceptions:

 # If the syntax of a regular expression is violated, a "SintaxTree_Error" exception is thrown.
 # When parsing a string, the only exception that can be thrown by std :: logic_error is when trying to access an uninitialized capture group.
=============================
	
# Most important objects:

 # An object of the "RgxResult" type stores the result of string analysis - an array of correct substrings
   that have successfully passed the analysis and, optionally, an array of tables with the values ​​of capture groups.

 # an object of type "Regular_expression" is a regular expression. 
  Constructed by string, if bool = true is passed to the constructor, an expression will be generated to invert the given string.
 	!!! This class also supports methods:
	  # std :: string restore_expression () - Returns the string representation of the regular expression. Since it is not stored explicitly,
	    but only in the form of an automaton (for the purpose of memory optimization), the returned expression will be many times longer than
	    the original one (restored by the K-path method), but it will be identical.
	    The automaton built according to this expression will also be identical to the initial one (minimal),
	    but it will take more time to build.For complex expressions, recovered will be very large,
	    to the point that recovery will be impossible and an exception will be thrown
	    std :: bad_alloc (this will not leak memory, everything is safe).

	  # Regular_expression make_language_inversion () - Returns a regular expression object that matches the inversion of the language.
             !!! NOTE: internally it uses restore_expression (), so for large expressions there may be problems,
	         for complex expressions use the Regular_expression string constructor with an additional parameter true
		 (this will build the inversion without having to restore the original regular expression)

	  # Regular_expression make_language_addition () noexcept - Returns a regular expression object that matches the language completion (safe).

~~~ The library also supports 2 functions for parsing strings and their overloading:

 # checkString - 
	1 argument - the parsed string,
	2 - argument - regular expression (either as a string or as an object),
	3 - argument (optional) RgxResult - if given, then the original string will be saved to it, if it passed the check successful and so is the capture group table (if any)
  The function returns bool - the result of the check.

 # findAll - Finds all maximum non-overlapping occurrences of the original string - substrings that match the regex
	1 - argument - parsed string
	2 - argument is a regular expression (either as a string or as an object)
	3 - argument (optional) - if true - as a result, a table of capture groups will also be returned for each substring that passed the check.
The function returns an RgxResult object.
===================================
	Also:

 # The library also supports the construction and printing of a syntax tree for a given string (class SintaxTree).

 # The class of a deterministic state machine, supports the same functions as the library,
   but you should not use it directly, it may be unsafe, it is better to use its wrapper in the form of Regular_Expression.

   	!!! (Multiplying two automata in this library will give the difference of 2 languages.
	    library functions and Regular_expression functions have been tested, the task of creating a
	    language difference was not worth it, but if you really need to create it, you can try
	    to use the product of automata explicitly, but this is not safe and can cause undefined behavior).
