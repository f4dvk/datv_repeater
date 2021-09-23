<?php

if(isset($_POST['exec']))
{
  if(isset($_GET['code']))
  {
    exec("/bin/echo '' > /home/jetson/datv_repeater/cmd.txt.tmp");
    $fichier = fopen('/home/jetson/datv_repeater/cmd.txt.tmp', 'w+');
    $code_dtmf = $_GET[code];

    exec("/bin/echo $code_dtmf > /home/jetson/datv_repeater/cmd.txt.tmp");

    //fseek($fichier, 0); // On remet le curseur au début du fichier
    //fputs($fichier, $code_dtmf); // On écrit le nouveau code
    //fprintf($fichier, $code_dtmf);

    //fclose($fichier);
    exec("/bin/mv /home/jetson/datv_repeater/cmd.txt.tmp /home/jetson/datv_repeater/cmd.txt");
    exec("sudo chmod 777 /home/jetson/datv_repeater/cmd.txt");
  }
}

if(isset($_POST['code_manu']))
{
  exec("/bin/echo '' > /home/jetson/datv_repeater/cmd.txt.tmp");
  $fichier = fopen('/home/jetson/datv_repeater/cmd.txt.tmp', 'w+');
  $code_dtmf = $_POST['code_manu'];

  exec("/bin/echo $code_dtmf > /home/jetson/datv_repeater/cmd.txt.tmp");

  //fseek($fichier, 0); // On remet le curseur au début du fichier
  //fputs($fichier, $code_dtmf); // On écrit le nouveau code

  //fclose($fichier);
  exec("/bin/mv /home/jetson/datv_repeater/cmd.txt.tmp /home/jetson/datv_repeater/cmd.txt");
  exec("sudo chmod 777 /home/jetson/datv_repeater/cmd.txt");
}

echo '
<body bgcolor="blue">
<!doctype html><html>
<center><head><title>Commande DATV</title></head>
<hr/><p><font color=yellow><strong> Commande Relais DATV </strong></font><p/><hr/>
<table><form method="post" action="tcm.php?code=*01">
<input type="submit" name="exec"  value="*01" />
</form>
<h>&nbsp;&nbsp;&nbsp;&nbsp;</h>
<form method="post" action="tcm.php?code=*02">
<input type="submit" name="exec"  value="*02" />
</form>
<h>&nbsp;&nbsp;&nbsp;&nbsp;</h>
<form method="post" action="tcm.php?code=*03">
<input type="submit" name="exec"  value="*03" />
</form></table><hr/>
<table><form method="post" action="tcm.php?code=A">
<input type="submit" name="exec"  value="Arret TX" />
</form>
<h>&nbsp;&nbsp;&nbsp;&nbsp;</h>
<form method="post" action="tcm.php?code=C">
<input type="submit" name="exec"  value="Etat Relais" />
</form>
<h>&nbsp;&nbsp;&nbsp;&nbsp;</h>
<form method="post" action="tcm.php?code=*99">
<input type="submit" name="exec"  value="Retour Phonie" />
</form></table><hr/>
<table><form method="post" action="tcm.php?code=*10">
<input type="submit" name="exec"  value="*10" />
</form>
<h>&nbsp;&nbsp;&nbsp;&nbsp;</h>
<form method="post" action="tcm.php?code=*11">
<input type="submit" name="exec"  value="*11" />
</form>
</form>
<h>&nbsp;&nbsp;&nbsp;&nbsp;</h>
<form method="post" action="tcm.php?code=*12">
<input type="submit" name="exec"  value="*12" />
</form>
<h>&nbsp;&nbsp;&nbsp;&nbsp;</h>
<form method="post" action="tcm.php?code=*13">
<input type="submit" name="exec"  value="*13" />
</form>
<h>&nbsp;&nbsp;&nbsp;&nbsp;</h>
<form method="post" action="tcm.php?code=*14">
<input type="submit" name="exec"  value="*14" />
</form>
<h>&nbsp;&nbsp;&nbsp;&nbsp;</h>
<form method="post" action="tcm.php?code=*15">
<input type="submit" name="exec"  value="*15" />
</form></table><hr/>
<p><font color=white><strong> Code libre </strong></font><p/>
<table><form method="post" action="tcm.php">
<input type="text" name="code_manu" />
<input type="submit" value="Valider" />
</form></table><hr/>';
?>
