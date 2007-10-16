<?

$body = <<<BODY
hello, {{ \$who }}!
{{ bye; }}

BODY;

class View extends Blitz {
    function bye() {
        return "Have a lot of fun!...";
    }
}

$T = new View();
$T->load($body);
echo $T->parse(array('who' => 'world'));

?>
