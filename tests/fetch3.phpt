--TEST--
fetch#3
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
    echo "FETCH: /root/a\n";
    echo $T->fetch('/root/a')."\n";
}

echo "\n\n";

$T->set_global(array(
    'x' => 'x_global', 
    'y' => 'y_global',
    'i' => 'i_global'
));

$i = 0;
$i_params = NULL;

$paths = array('/root/a', '/root', '/', '/root/a/b'); 

$T->context('/root/a/b');
$T->set(array('y' => 'yast'));
$T->context('/root');
$T->set(array('p1' => 'alpha', 'p2' => 'omega'));
$T->context('/root/a');
$T->set(array('x' => 'xenix'));
$T->context('/');
$T->set(array('title' => 'complex fetch #1'));
$T->context('/root/a');
echo $T->fetch('/', array('title' => 'complex fetch #1'));

$T->context('/root/a/b');
$T->set(array('y' => 'yello'));
echo $T->fetch('/root');

echo $T->fetch('/',array('title' => 'complex fetch #2'));

?>
--EXPECT--
FETCH: /root/a

A-begin(0)  ; x=x_value#0, y= A-end

FETCH: /root/a

A-begin(1)  B-begin; y=y_value#1 B-end ; x=x_value#1, y= A-end

FETCH: /root/a

A-begin(2)  ; x=x_value#2, y= A-end

FETCH: /root/a

A-begin(3)  B-begin; y=y_value#3 B-end ; x=x_value#3, y= A-end

FETCH: /root/a

A-begin(4)  ; x=x_value#4, y= A-end

FETCH: /root/a

A-begin(5)  B-begin; y=y_value#5 B-end ; x=x_value#5, y= A-end

FETCH: /root/a

A-begin(6)  ; x=x_value#6, y= A-end

FETCH: /root/a

A-begin(7)  B-begin; y=y_value#7 B-end ; x=x_value#7, y= A-end

FETCH: /root/a

A-begin(8)  ; x=x_value#8, y= A-end

FETCH: /root/a

A-begin(9)  B-begin; y=y_value#9 B-end ; x=x_value#9, y= A-end



complex fetch example complex fetch #1

this is root alpha

A-begin(i_global)  B-begin; y=yast B-end ; x=xenix, y=y_global A-end

end of root omega


this is root 

A-begin(i_global)  B-begin; y=yello B-end ; x=x_global, y=y_global A-end

end of root 
complex fetch example complex fetch #2


