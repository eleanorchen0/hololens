using UnityEngine;

[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class DipoleAntenna : MonoBehaviour
{
    [Header("resolution")]
    public int phiSegments = 64;
    public int thetaSegments = 32;

    [Header("scaling")]
    public float radiusScale = 1f;

    private Mesh mesh;

    void Awake()
    {
        mesh = new Mesh();
        mesh.name = "DipoleAntenna";
        GetComponent<MeshFilter>().mesh = mesh;
        BuildMesh();
    }

    void OnValidate()
    {
        if (mesh != null)
            BuildMesh();
    }

    void BuildMesh()
    {
        mesh.Clear();

        int vertCount = (phiSegments + 1) * (thetaSegments + 1);
        Vector3[] vertices = new Vector3[vertCount];
        Vector2[] uvs = new Vector2[vertCount];
        int[] triangles = new int[phiSegments * thetaSegments * 6];

        int vi = 0;
        for (int ti = 0; ti <= thetaSegments; ti++)
        {
            float theta = Mathf.PI * ti / thetaSegments;
            float sinT = Mathf.Sin(theta);
            float cosT = Mathf.Cos(theta);

            float r = radiusScale * sinT;

            for (int pi = 0; pi <= phiSegments; pi++)
            {
                //***************CALCS**********************
                // use to change parameters??
                float phi = 2f * Mathf.PI * pi / phiSegments;
                float cosP = Mathf.Cos(phi);
                float sinP = Mathf.Sin(phi);

                float x = r * sinT * cosP;
                float y = r * cosT;
                float z = r * sinT * sinP;
                vertices[vi] = new Vector3(x, y, z);

                uvs[vi] = new Vector2(pi / (float)phiSegments, sinT * sinT);

                vi++;
            }
        }

        int tii = 0;
        for (int ti = 0; ti < thetaSegments; ti++)
        {
            for (int pi = 0; pi < phiSegments; pi++)
            {
                int a = ti * (phiSegments + 1) + pi;
                int b = (ti + 1) * (phiSegments + 1) + pi;
                int c = (ti + 1) * (phiSegments + 1) + pi + 1;
                int d = ti * (phiSegments + 1) + pi + 1;

                triangles[tii++] = a;
                triangles[tii++] = b;
                triangles[tii++] = c;

                triangles[tii++] = a;
                triangles[tii++] = c;
                triangles[tii++] = d;
            }
        }

        mesh.vertices = vertices;
        mesh.uv = uvs;
        mesh.triangles = triangles;
        mesh.RecalculateNormals();
    }

    public void UpdateParams(float radius)
    {
        radiusScale = radius;
        BuildMesh();
    }
}