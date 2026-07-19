subshader "r_rocket_m1_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 1 1 1;
	materialSpecular 0.156863 0.156863 0.156863;
	materialSpecularPower 13;
	envmap true;
	texture "texture/katy_02_o";
}

subshader "r_rocket_m1_Material1" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 1 1 1;
	materialSpecular 0.156863 0.156863 0.156863;
	materialSpecularPower 12.5;
	twosided true;
	envmap true;
	texture "texture/katy_02_o";
}

