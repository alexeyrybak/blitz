--TEST--
mix #6
--FILE--
<?php
include('common.inc');

$T = new Blitz('fetch_cmplx.tpl');

for($i=0; $i<10; $i++) {
    $T->context('/root/a');
    $T->set(array('x' => 'x_value#'.$i, 'i'=> $i));
    if($i%2) {
        $T->context('b');
        $T->set(array('y' => 'y_value#'.$i));
    }
}

$T->dump_iterations();

?>
--EXPECT--
ITERATION DUMP (4 parts)
(1) iterations:
array(1) {
  [0]=>
  array(1) {
    ["root"]=>
    array(1) {
      [0]=>
      array(1) {
        ["a"]=>
        array(1) {
          [0]=>
          array(3) {
            ["x"]=>
            string(9) "x_value#9"
            ["i"]=>
            int(9)
            ["b"]=>
            array(1) {
              [0]=>
              array(1) {
                ["y"]=>
                string(9) "y_value#9"
              }
            }
          }
        }
      }
    }
  }
}
(2) current path is: /root/a/b
(3) current node data (current_iteration_parent) is:
array(1) {
  [0]=>
  array(1) {
    ["root"]=>
    array(1) {
      [0]=>
      array(1) {
        ["a"]=>
        array(1) {
          [0]=>
          array(3) {
            ["x"]=>
            string(9) "x_value#9"
            ["i"]=>
            int(9)
            ["b"]=>
            array(1) {
              [0]=>
              array(1) {
                ["y"]=>
                string(9) "y_value#9"
              }
            }
          }
        }
      }
    }
  }
}
(4) current node item data (current_iteration) is:
empty

