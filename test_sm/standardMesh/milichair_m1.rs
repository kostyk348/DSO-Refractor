subshader "milichair_m1_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 1 1 1;
	materialSpecular 0 0 0;
	materialSpecularPower 12.5;
	texture "texture/chair1_s";
}

subshader "milichair_m1_Material1" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular true;
	materialDiffuse 1 1 1;
	materialSpecular 0.478431 0.478431 0.478431;
	materialSpecularPower 21;
	envmap true;
	texture "texture/chair1_s";
}

