subshader "Willy_Hul_M1_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	texture "texture/Willy2_Z";
}

subshader "Willy_Hul_M1_Material1" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	texture "texture/Willy1_Z";
}

subshader "Willy_Hul_M1_Material2" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	transparent true;
	twosided true;
    depthWrite false;
	envmap true;
	texture "texture/Willy3_Z";
}

