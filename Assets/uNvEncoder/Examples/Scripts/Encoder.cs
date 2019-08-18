using UnityEngine;
using UnityEngine.Assertions;
using System.Collections;

namespace uNvEncoder.Examples
{

public class Encoder : MonoBehaviour
{
    [SerializeField]
    uNvEncoderEncode encoder = null;

    [SerializeField]
    RenderTexture texture = null;

    public bool forceIdrFrame = true;

    IEnumerator Start()
    {
        Assert.IsNotNull(encoder);
        Assert.IsNotNull(texture);
        Assert.AreEqual<int>(encoder.width, texture.width);
        Assert.AreEqual<int>(encoder.height, texture.height);

        for (;;)
        {
            yield return new WaitForEndOfFrame();

            if (!encoder.Encode(texture, forceIdrFrame))
            {
                Debug.LogError("Encode() failed.");
            }
        }
    }
}

}
