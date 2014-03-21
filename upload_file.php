<?php

shell_exec("rm ./upload/*");

$flag = 0;
$allowedExts = array("gif", "jpeg", "jpg", "png" , "pgm", "ppm");
$extension = end(explode(".", $_FILES["file1"]["name"]));
if ((($_FILES["file1"]["type"] == "image/gif")
			|| ($_FILES["file1"]["type"] == "image/jpeg")
			|| ($_FILES["file1"]["type"] == "image/jpg")
			|| ($_FILES["file1"]["type"] == "image/png"))
		|| in_array($extension, $allowedExts))
{
	if ($_FILES["file1"]["error"] > 0)
	{
		echo "Return Code: " . $_FILES["file1"]["error"] . "<br>";
	}
	else
	{
	/*	echo "Upload: " . $_FILES["file1"]["name"] . "<br>";
		echo "Type: " . $_FILES["file1"]["type"] . "<br>";
		echo "Size: " . ($_FILES["file1"]["size"] / 1024) . " kB<br>";
		echo "Temp file: " . $_FILES["file1"]["tmp_name"] . "<br>";
*/
		if (file_exists("/var/www/ass3/upload/" . $_FILES["file1"]["name"]))
		{
			echo $_FILES["file1"]["name"] . " already exists. ";
		}
		else
		{
			move_uploaded_file($_FILES["file1"]["tmp_name"],
					"/var/www/ass3/upload/" . $_FILES["file1"]["name"]);
//			echo "Stored in: " . "upload/" . $_FILES["file1"]["name"] . "<br>";
		}
	}
}
else
{
	$flag = 1;
	echo "Invalid file";
}

$extension = end(explode(".", $_FILES["file2"]["name"]));
if ((($_FILES["file2"]["type"] == "image/gif")
			|| ($_FILES["file2"]["type"] == "image/jpeg")
			|| ($_FILES["file2"]["type"] == "image/jpg")
			|| ($_FILES["file2"]["type"] == "image/png"))
		|| in_array($extension, $allowedExts))
{
	if ($_FILES["file2"]["error"] > 0)
	{
		echo "Return Code: " . $_FILES["file2"]["error"] . "<br>";
	}
	else
	{
/*		echo "<br>";
		echo "Upload: " . $_FILES["file2"]["name"] . "<br>";
		echo "Type: " . $_FILES["file2"]["type"] . "<br>";
		echo "Size: " . ($_FILES["file2"]["size"] / 1024) . " kB<br>";
		echo "Temp file: " . $_FILES["file2"]["tmp_name"] . "<br>";
*/
		if (file_exists("/var/www/ass3/upload/" . $_FILES["file2"]["name"]))
		{
			echo $_FILES["file2"]["name"] . " already exists. ";
		}
		else
		{
			move_uploaded_file($_FILES["file2"]["tmp_name"],
					"/var/www/ass3/upload/" . $_FILES["file2"]["name"]);
//			echo "Stored in: " . "upload/" . $_FILES["file2"]["name"] . "<br>";
		}
	}
}
else
{
	$flag = 1;
	echo "Invalid file";
}

if ($flag == 0){

	$desc = $_POST["Descriptor"];
	$ext = $_POST['Extractor'];
	$mat = $_POST['Matcher'];
	$file1 = "upload/".$_FILES['file1']['name'];
	$file2 = "upload/".$_FILES['file2']['name'];
	$pan = $_POST['Panorama'];

	echo "File Upload Successful" . "<br>";

/*	echo $desc . "<br>";
	echo $_POST['Extractor']. "<br>";
	echo $_POST['Matcher']. "<br>";
	echo $_FILES['file1']['name']. "<br>";
	echo $_FILES['file2']['name']. "<br>";*/

	$out = shell_exec("./det $file1 $file2 $desc $ext $mat $pan");
//	if($pan == "Yes"){
//		shell_exec("./pan $file1 $file2");
//	}
	$len = strlen($out);
	echo '<html>';
	echo '<body><p>';
	for ($i=0;$i<$len;$i++){
		if($out[$i] == "#")
			echo "<br>";
		else{
			echo $out[$i];
			if($out[$i] ==';')
				echo "<br>";
		}
	}
	echo '</p>';
	echo '<table width="230" border="10" align="left" cellpadding="5" cellspacing="10">
		<tr>
		<td>
		<img src="upload/epilines.png" alt="Epipolar Lines">
		</td></tr><tr>
		<td>
		<h3>Epipolar Lines</h3>
		</td>
		</tr>
		<tr>
		<td>
		<img src="upload/matches.png" alt="Matches">
		</td></tr><tr>
		<td>
		<h3>Matches</h3>
		</td>
		</tr>
		<tr>
		<td>
		<img src="upload/hom.png" alt="Homography">
		</td></tr><tr>
		<td>
		<h3>Object Aligned to Scene Image</h3>
		</td>
		</tr>';

	if($pan == "Yes"){
		echo '<tr>
			<td>
			<img src="upload/pan.png" alt="Panorama">
			</td></tr>
			<tr>
			<td>
			<h3>Panorama</h3>
			</td>
			</tr>';
	}

	echo '</table></body></html>';
}
?>
