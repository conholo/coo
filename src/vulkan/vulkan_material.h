class VulkanMaterial
{
public:
    VulkanMaterial(std::shared_ptr<VulkanShader> vertexShader, std::shared_ptr<VulkanShader> fragmentShader);

    void CreatePipelineLayout(ShaderResourceManager &resourceManager);
    void AllocateDescriptorSets(ShaderResourceManager &resourceManager);
    void UpdateDescriptorSet(uint32_t set, const std::vector <VkWriteDescriptorSet> &writes);
    void SetPushConstants(void *data, size_t size);

private:
    std::shared_ptr <Shader> m_VertexShader;
    std::shared_ptr <Shader> m_FragmentShader;
    VkPipelineLayout m_PipelineLayout;
    std::vector <VkDescriptorSet> m_DescriptorSets;
    std::vector<uint8_t> m_PushConstantData;
};