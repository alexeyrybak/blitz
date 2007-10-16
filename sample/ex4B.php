<?

$T = new Blitz('ex4A.tpl');
$character = array(
    array(1,'The Dude',0),
    array(2,'Walter Sobchak',0),
    array(3,'Donny',1),
    array(4,'Maude Lebowski',0),
    array(5,'The Big Lebowski',0),
    array(6,'Brandt',0),
    array(7,'Jesus Quintana',0),
);

foreach ($character as $i => $data) {
   echo $T->parse(
       array(
           'num' => $data[0], 
           'name' => $data[1], 
           'rip' => $data[2]
       )
   );
}

?>
