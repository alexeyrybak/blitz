--TEST--
complex conditions 2
--FILE--
<?php
include('common.inc');
ini_set('blitz.remove_spaces_around_context_tags', 1);

$body = <<<BODY
Complex conditions
{{IF ((a == "1" || a == "2") && b == "3")}}
  A is one or twoo WHILE b is three
{{ELSE}}
  A is not one or twoo OR b is not three
{{END}}

{{IF a || b || c || !d}}
  A OR B OR C OR NOT(D)
{{ELSE}}
  NOP MATE
{{END}}

{{ IF !c }}NOT C{{ ELSE }}not NOT C{{ END }}
{{ UNLESS !a }}not NOT A{{ ELSE }}NOT A{{ END }}
{{ IF !true }}NOT TRUE{{ ELSE }}not NOT TRUE{{ END }}
{{ IF !false }}NOT FALSE{{ ELSE }}not NOT FALSE{{ END }}

###

BODY;

$T = new Blitz();
$T->load($body);
$T->display(
    array(
        'a' => 2,
        'b' => 3,
        'c' => false
    )
);
$T->display(
    array(
        'a' => 0,
        'b' => 0,
        'c' => false
    )
);
$T->display(
    array(
        'd' => true
    )
);
$T->display(
    array(
        'a' => 0,
        'b' => 0,
        'c' => false,
        'd' => false
    )
);

?>
--EXPECT--
Complex conditions
  A is one or twoo WHILE b is three

  A OR B OR C OR NOT(D)

NOT C
not NOT A
not NOT TRUE
NOT FALSE

###
Complex conditions
  A is not one or twoo OR b is not three

  A OR B OR C OR NOT(D)

NOT C
NOT A
not NOT TRUE
NOT FALSE

###
Complex conditions
  A is not one or twoo OR b is not three

  NOP MATE

NOT C
NOT A
not NOT TRUE
NOT FALSE

###
Complex conditions
  A is not one or twoo OR b is not three

  A OR B OR C OR NOT(D)

NOT C
NOT A
not NOT TRUE
NOT FALSE

###

