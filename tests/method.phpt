--TEST--
user-defined methods
--FILE--
<?php
include('common.inc');

class BlitzTemplate extends Blitz {
    var $titem;

    function BlitzTemplate($t) {
        parent::Blitz($t);
        $this->set(array(
            'test'=>'var_test',
            'x' => 2005,
            'y' => 2006
        ));
    }

    function test() {
        return "OK";
    }

    function test_arg($p1,$p2,$p3,$p4,$p5) {
        $result = 'user method called ('.strtolower(__CLASS__).','.__LINE__.')'."\n";
        $result .= 'parameters are:'."\n";
        $result .= '1:'.var_export($p1,TRUE)."\n";
        $result .= '2:'.var_export($p2,TRUE)."\n";
        $result .= '3:'.var_export($p3,TRUE)."\n";
        $result .= '4:'.var_export($p4,TRUE)."\n";
        $result .= '5:'.var_export($p5,TRUE)."\n";
        return $result;
    }
}

$T = new BlitzTemplate('method.tpl');
echo $T->parse();

?>
--EXPECT--
(1) method without brackets is variable: var_test
(2) method (brackets) OK
(3) method (brackets,semicolon) OK
(4) method (semicolon) without brackets is variable: var_test
(5) special variable for alternative format: var_test
(5) special variable for alternative format (semicolon): var_test
(5) special variable for alternative format (noprefix): var_test
(5) special variable for alternative format (noprefix,semicolon): var_test
(7) method for alternative format (brackets): OK
(8) method for alternative format (brackets, semicolon): OK
(9) method with arguments: user method called (blitztemplate,21)
parameters are:
1:134
2:2005
3:'hello,world!'
4:NULL
5:2006
