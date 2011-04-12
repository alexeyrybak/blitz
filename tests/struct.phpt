--TEST--
context struct
--FILE--
<?php

include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);
ini_set('blitz.warn_context_duplicates', 0);

$body = <<<BODY
{{ BEGIN some }}
    some began
    {{ IF bla }}
        <if>
        {{ BEGIN bla }}
            item = {{ item }}
        {{ END }}
        {{ BEGIN bla }}
            item = {{ item }}
        {{ END }}
        </if>
    {{ END }}
    {{ IF bla }} bla again {{ END }}
    {{ UNLESS dummy }}
    unless worked: {{ var }}
    {{ END }}
    {{ UNLESS dummy }}
    unless worked: {{ var }}
    {{ END }}
    some ended
{{ END }}
{{ BEGIN some }}
    some again
{{ END }}
BODY;

$T = new Blitz();
$T->load($body);
var_dump($T->getStruct());

?>
--EXPECT--
array(20) {
  [0]=>
  string(6) "/some/"
  [1]=>
  string(9) "/some/IF/"
  [2]=>
  string(13) "/some/IF/bla/"
  [3]=>
  string(17) "/some/IF/bla/item"
  [4]=>
  string(16) "/some/IF/bla/END"
  [5]=>
  string(13) "/some/IF/bla/"
  [6]=>
  string(17) "/some/IF/bla/item"
  [7]=>
  string(16) "/some/IF/bla/END"
  [8]=>
  string(12) "/some/IF/END"
  [9]=>
  string(9) "/some/IF/"
  [10]=>
  string(12) "/some/IF/END"
  [11]=>
  string(13) "/some/UNLESS/"
  [12]=>
  string(16) "/some/UNLESS/var"
  [13]=>
  string(16) "/some/UNLESS/END"
  [14]=>
  string(13) "/some/UNLESS/"
  [15]=>
  string(16) "/some/UNLESS/var"
  [16]=>
  string(16) "/some/UNLESS/END"
  [17]=>
  string(9) "/some/END"
  [18]=>
  string(6) "/some/"
  [19]=>
  string(9) "/some/END"
}
