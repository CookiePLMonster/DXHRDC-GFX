#include "WrappedDevice.h"

#include <utility>


HRESULT WINAPI D3D11CreateDevice_Export( IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags,
                        const D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, ID3D11Device** ppDevice,
                        D3D_FEATURE_LEVEL* pFeatureLevel, ID3D11DeviceContext** ppImmediateContext )
{
    wil::assign_null_to_opt_param(ppDevice);
    wil::assign_null_to_opt_param(ppImmediateContext);

    // Try to load a real d3d11.dll, D3D11Device will claim ownership of its reference if created successfully
    wil::unique_hmodule d3dModule( LoadLibrary(L"d3d11") );
    if ( d3dModule.is_valid() )
    {
        PFN_D3D11_CREATE_DEVICE createFn = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>(GetProcAddress( d3dModule.get(), "D3D11CreateDevice" ));
        if ( createFn != nullptr )
        {
            ComPtr<ID3D11Device> device;
            ComPtr<ID3D11DeviceContext> immediateContext;
            HRESULT hr = createFn( pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, device.GetAddressOf(), pFeatureLevel, immediateContext.GetAddressOf() );
            if ( SUCCEEDED(hr) )
            {
                ComPtr<ID3D11Device> wrappedDevice = Make<D3D11Device>( std::move(d3dModule), std::move(device), std::move(immediateContext) );

                ComPtr<ID3D11DeviceContext> wrappedDeviceContext;
                wrappedDevice->GetImmediateContext(wrappedDeviceContext.GetAddressOf());

                wil::detach_to_opt_param(ppDevice, wrappedDevice);
                wil::detach_to_opt_param(ppImmediateContext, wrappedDeviceContext);
            }
            return hr;
        }
    }

    return DXGI_ERROR_SDK_COMPONENT_MISSING; // If either LoadLibrary or GetProcAddress fails
}

// ====================================================

D3D11Device::D3D11Device(wil::unique_hmodule module, ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> immediateContext)
    : m_d3dModule( std::move(module) ), m_orig( std::move(device) )
{
    ComPtr<D3D11DeviceContext> context = Make<D3D11DeviceContext>( std::move(immediateContext), this );
    m_immediateContext = context.Detach();
}

ULONG STDMETHODCALLTYPE D3D11Device::Release()
{
    ULONG ref = __super::Release();
    if ( ref == 1 ) // Immediate context holds a reference to this object too
    {
        m_immediateContext->Release();
    }
    return ref;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateBuffer(const D3D11_BUFFER_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Buffer** ppBuffer)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateTexture1D(const D3D11_TEXTURE1D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture1D** ppTexture1D)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateTexture2D(const D3D11_TEXTURE2D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D** ppTexture2D)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateTexture3D(const D3D11_TEXTURE3D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture3D** ppTexture3D)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateShaderResourceView(ID3D11Resource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc, ID3D11ShaderResourceView** ppSRView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateUnorderedAccessView(ID3D11Resource* pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc, ID3D11UnorderedAccessView** ppUAView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateRenderTargetView(ID3D11Resource* pResource, const D3D11_RENDER_TARGET_VIEW_DESC* pDesc, ID3D11RenderTargetView** ppRTView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateDepthStencilView(ID3D11Resource* pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc, ID3D11DepthStencilView** ppDepthStencilView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs, UINT NumElements, const void* pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, ID3D11InputLayout** ppInputLayout)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateVertexShader(const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11VertexShader** ppVertexShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateGeometryShader(const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11GeometryShader** ppGeometryShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateGeometryShaderWithStreamOutput(const void* pShaderBytecode, SIZE_T BytecodeLength, const D3D11_SO_DECLARATION_ENTRY* pSODeclaration, UINT NumEntries, const UINT* pBufferStrides, UINT NumStrides, UINT RasterizedStream, ID3D11ClassLinkage* pClassLinkage, ID3D11GeometryShader** ppGeometryShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreatePixelShader(const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11PixelShader** ppPixelShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateHullShader(const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11HullShader** ppHullShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateDomainShader(const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11DomainShader** ppDomainShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateComputeShader(const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pClassLinkage, ID3D11ComputeShader** ppComputeShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateClassLinkage(ID3D11ClassLinkage** ppLinkage)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateBlendState(const D3D11_BLEND_DESC* pBlendStateDesc, ID3D11BlendState** ppBlendState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc, ID3D11DepthStencilState** ppDepthStencilState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateRasterizerState(const D3D11_RASTERIZER_DESC* pRasterizerDesc, ID3D11RasterizerState** ppRasterizerState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateSamplerState(const D3D11_SAMPLER_DESC* pSamplerDesc, ID3D11SamplerState** ppSamplerState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateQuery(const D3D11_QUERY_DESC* pQueryDesc, ID3D11Query** ppQuery)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreatePredicate(const D3D11_QUERY_DESC* pPredicateDesc, ID3D11Predicate** ppPredicate)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateCounter(const D3D11_COUNTER_DESC* pCounterDesc, ID3D11Counter** ppCounter)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CreateDeferredContext(UINT ContextFlags, ID3D11DeviceContext** ppDeferredContext)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::OpenSharedResource(HANDLE hResource, REFIID ReturnedInterface, void** ppResource)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CheckFormatSupport(DXGI_FORMAT Format, UINT* pFormatSupport)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CheckMultisampleQualityLevels(DXGI_FORMAT Format, UINT SampleCount, UINT* pNumQualityLevels)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE D3D11Device::CheckCounterInfo(D3D11_COUNTER_INFO* pCounterInfo)
{
}

HRESULT STDMETHODCALLTYPE D3D11Device::CheckCounter(const D3D11_COUNTER_DESC* pDesc, D3D11_COUNTER_TYPE* pType, UINT* pActiveCounters, LPSTR szName, UINT* pNameLength, LPSTR szUnits, UINT* pUnitsLength, LPSTR szDescription, UINT* pDescriptionLength)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::CheckFeatureSupport(D3D11_FEATURE Feature, void* pFeatureSupportData, UINT FeatureSupportDataSize)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::GetPrivateData(REFGUID guid, UINT* pDataSize, void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::SetPrivateData(REFGUID guid, UINT DataSize, const void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11Device::SetPrivateDataInterface(REFGUID guid, const IUnknown* pData)
{
    return E_NOTIMPL;
}

D3D_FEATURE_LEVEL STDMETHODCALLTYPE D3D11Device::GetFeatureLevel(void)
{
    return D3D_FEATURE_LEVEL();
}

UINT STDMETHODCALLTYPE D3D11Device::GetCreationFlags(void)
{
    return 0;
}

HRESULT STDMETHODCALLTYPE D3D11Device::GetDeviceRemovedReason(void)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE D3D11Device::GetImmediateContext(ID3D11DeviceContext** ppImmediateContext)
{
    m_immediateContext->AddRef();
    *ppImmediateContext = m_immediateContext;
}

HRESULT STDMETHODCALLTYPE D3D11Device::SetExceptionMode(UINT RaiseFlags)
{
    return E_NOTIMPL;
}

UINT STDMETHODCALLTYPE D3D11Device::GetExceptionMode(void)
{
    return 0;
}

// ====================================================

inline D3D11DeviceContext::D3D11DeviceContext(ComPtr<ID3D11DeviceContext> context, ComPtr<D3D11Device> device)
    : m_device(std::move(device)), m_orig(std::move(context))
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::GetDevice(ID3D11Device** ppDevice)
{
}

HRESULT STDMETHODCALLTYPE D3D11DeviceContext::GetPrivateData(REFGUID guid, UINT* pDataSize, void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceContext::SetPrivateData(REFGUID guid, UINT DataSize, const void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceContext::SetPrivateDataInterface(REFGUID guid, const IUnknown* pData)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE D3D11DeviceContext::VSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::PSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::PSSetShader(ID3D11PixelShader* pPixelShader, ID3D11ClassInstance* const* ppClassInstances, UINT NumClassInstances)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::PSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::VSSetShader(ID3D11VertexShader* pVertexShader, ID3D11ClassInstance* const* ppClassInstances, UINT NumClassInstances)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::Draw(UINT VertexCount, UINT StartVertexLocation)
{
}

HRESULT STDMETHODCALLTYPE D3D11DeviceContext::Map(ID3D11Resource* pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE D3D11DeviceContext::Unmap(ID3D11Resource* pResource, UINT Subresource)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::PSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::IASetInputLayout(ID3D11InputLayout* pInputLayout)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::IASetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppVertexBuffers, const UINT* pStrides, const UINT* pOffsets)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::IASetIndexBuffer(ID3D11Buffer* pIndexBuffer, DXGI_FORMAT Format, UINT Offset)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::GSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::GSSetShader(ID3D11GeometryShader* pShader, ID3D11ClassInstance* const* ppClassInstances, UINT NumClassInstances)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY Topology)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::VSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::VSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::Begin(ID3D11Asynchronous* pAsync)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::End(ID3D11Asynchronous* pAsync)
{
}

HRESULT STDMETHODCALLTYPE D3D11DeviceContext::GetData(ID3D11Asynchronous* pAsync, void* pData, UINT DataSize, UINT GetDataFlags)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE D3D11DeviceContext::SetPredication(ID3D11Predicate* pPredicate, BOOL PredicateValue)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::GSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::GSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::OMSetRenderTargets(UINT NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::OMSetRenderTargetsAndUnorderedAccessViews(UINT NumRTVs, ID3D11RenderTargetView* const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView* const* ppUnorderedAccessViews, const UINT* pUAVInitialCounts)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::OMSetBlendState(ID3D11BlendState* pBlendState, const FLOAT BlendFactor[4], UINT SampleMask)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::OMSetDepthStencilState(ID3D11DepthStencilState* pDepthStencilState, UINT StencilRef)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::SOSetTargets(UINT NumBuffers, ID3D11Buffer* const* ppSOTargets, const UINT* pOffsets)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::DrawAuto(void)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::DrawIndexedInstancedIndirect(ID3D11Buffer* pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::DrawInstancedIndirect(ID3D11Buffer* pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::DispatchIndirect(ID3D11Buffer* pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::RSSetState(ID3D11RasterizerState* pRasterizerState)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::RSSetViewports(UINT NumViewports, const D3D11_VIEWPORT* pViewports)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::RSSetScissorRects(UINT NumRects, const D3D11_RECT* pRects)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::CopySubresourceRegion(ID3D11Resource* pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource* pSrcResource, UINT SrcSubresource, const D3D11_BOX* pSrcBox)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::CopyResource(ID3D11Resource* pDstResource, ID3D11Resource* pSrcResource)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::UpdateSubresource(ID3D11Resource* pDstResource, UINT DstSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::CopyStructureCount(ID3D11Buffer* pDstBuffer, UINT DstAlignedByteOffset, ID3D11UnorderedAccessView* pSrcView)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::ClearRenderTargetView(ID3D11RenderTargetView* pRenderTargetView, const FLOAT ColorRGBA[4])
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::ClearUnorderedAccessViewUint(ID3D11UnorderedAccessView* pUnorderedAccessView, const UINT Values[4])
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::ClearUnorderedAccessViewFloat(ID3D11UnorderedAccessView* pUnorderedAccessView, const FLOAT Values[4])
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::ClearDepthStencilView(ID3D11DepthStencilView* pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::GenerateMips(ID3D11ShaderResourceView* pShaderResourceView)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::SetResourceMinLOD(ID3D11Resource* pResource, FLOAT MinLOD)
{
}

FLOAT STDMETHODCALLTYPE D3D11DeviceContext::GetResourceMinLOD(ID3D11Resource* pResource)
{
    return FLOAT();
}

void STDMETHODCALLTYPE D3D11DeviceContext::ResolveSubresource(ID3D11Resource* pDstResource, UINT DstSubresource, ID3D11Resource* pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::ExecuteCommandList(ID3D11CommandList* pCommandList, BOOL RestoreContextState)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::HSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::HSSetShader(ID3D11HullShader* pHullShader, ID3D11ClassInstance* const* ppClassInstances, UINT NumClassInstances)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::HSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::HSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::DSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::DSSetShader(ID3D11DomainShader* pDomainShader, ID3D11ClassInstance* const* ppClassInstances, UINT NumClassInstances)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::DSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::DSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::CSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::CSSetUnorderedAccessViews(UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView* const* ppUnorderedAccessViews, const UINT* pUAVInitialCounts)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::CSSetShader(ID3D11ComputeShader* pComputeShader, ID3D11ClassInstance* const* ppClassInstances, UINT NumClassInstances)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::CSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::CSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::VSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::PSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::PSGetShader(ID3D11PixelShader** ppPixelShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::PSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::VSGetShader(ID3D11VertexShader** ppVertexShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::PSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::IAGetInputLayout(ID3D11InputLayout** ppInputLayout)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::IAGetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppVertexBuffers, UINT* pStrides, UINT* pOffsets)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::IAGetIndexBuffer(ID3D11Buffer** pIndexBuffer, DXGI_FORMAT* Format, UINT* Offset)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::GSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::GSGetShader(ID3D11GeometryShader** ppGeometryShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::IAGetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY* pTopology)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::VSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::VSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::GetPredication(ID3D11Predicate** ppPredicate, BOOL* pPredicateValue)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::GSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::GSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::OMGetRenderTargets(UINT NumViews, ID3D11RenderTargetView** ppRenderTargetViews, ID3D11DepthStencilView** ppDepthStencilView)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::OMGetRenderTargetsAndUnorderedAccessViews(UINT NumRTVs, ID3D11RenderTargetView** ppRenderTargetViews, ID3D11DepthStencilView** ppDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView** ppUnorderedAccessViews)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::OMGetBlendState(ID3D11BlendState** ppBlendState, FLOAT BlendFactor[4], UINT* pSampleMask)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::OMGetDepthStencilState(ID3D11DepthStencilState** ppDepthStencilState, UINT* pStencilRef)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::SOGetTargets(UINT NumBuffers, ID3D11Buffer** ppSOTargets)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::RSGetState(ID3D11RasterizerState** ppRasterizerState)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::RSGetViewports(UINT* pNumViewports, D3D11_VIEWPORT* pViewports)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::RSGetScissorRects(UINT* pNumRects, D3D11_RECT* pRects)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::HSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::HSGetShader(ID3D11HullShader** ppHullShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::HSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::HSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::DSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::DSGetShader(ID3D11DomainShader** ppDomainShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::DSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::DSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::CSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::CSGetUnorderedAccessViews(UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView** ppUnorderedAccessViews)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::CSGetShader(ID3D11ComputeShader** ppComputeShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::CSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::CSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::ClearState(void)
{
}

void STDMETHODCALLTYPE D3D11DeviceContext::Flush(void)
{
}

D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE D3D11DeviceContext::GetType(void)
{
    return D3D11_DEVICE_CONTEXT_TYPE();
}

UINT STDMETHODCALLTYPE D3D11DeviceContext::GetContextFlags(void)
{
    return 0;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceContext::FinishCommandList(BOOL RestoreDeferredContextState, ID3D11CommandList** ppCommandList)
{
    return E_NOTIMPL;
}
