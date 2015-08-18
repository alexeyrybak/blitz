--TEST--
Test that ifs work in relatively complex situations
--FILE--
<?php
include('common.inc');

error_reporting(E_ALL);
set_error_handler('my_error_handler');
function my_error_handler($errno, $errstr, $errfile, $errline) {
    echo "ERROR: " . rtrim($errstr) . "\n";
}

$body = <<<BODY
Before block1
{{BEGIN BLOCK1}}
Before cb1
{{IF cb1(a, b)}}
Start of cb1 a={{a}} and b={{b}}
{{BEGIN BLOCK2}}
Contents of block2 if cb1
{{END BLOCK2}}
End of cb1
{{ELSEIF cb2(c, d)}}
Start of cb2 c={{c}} and d={{d}}
{{BEGIN BLOCK2}}
Contents of block2 if cb2
{{END BLOCK2}}
{{END}}
Before block1 end
{{END BLOCK1}}
After block1 end
BODY;

class View extends Blitz {
	public function cb1($a, $b) {
		return $a == 3 && $b == 4;
	}
	public function cb2($a, $b) {
		return $a == 5 && $b == 6;
	}
}

$T = new View();
$T->load($body);

$T->display(
	array('BLOCK1' => array(array('a' => 3, 'b' => 4, 'c' => 5, 'd' => 6, 'BLOCK2' => array(array()))))
);

echo "\n///Callback2///\n";

$T->display(
	array('BLOCK1' => array(array('a' => 4, 'b' => 1, 'c' => 5, 'd' => 6, 'BLOCK2' => array(array()))))
);

?>
--EXPECT--
Before block1

Before cb1

Start of cb1 a=3 and b=4

Contents of block2 if cb1

End of cb1

Before block1 end

After block1 end
///Callback2///
Before block1

Before cb1

Start of cb2 c=5 and d=6

Contents of block2 if cb2


Before block1 end

After block1 end