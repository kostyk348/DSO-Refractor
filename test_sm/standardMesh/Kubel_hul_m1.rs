subshader "Kubel_hul_m1_Material0" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	texture "texture/Kubel1_Z";
}

subshader "Kubel_hul_m1_Material1" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	texture "texture/Kubel2_Z";
}

subshader "Kubel_hul_m1_Material3" "StandardMesh/Default"
{
	lighting true;
	lightingSpecular false;
	materialDiffuse 1 1 1;
	transparent true;
	twosided true;
    depthWrite false;
	envmap true;
	texture "texture/Kubel_Willy_Window_Z";
}

