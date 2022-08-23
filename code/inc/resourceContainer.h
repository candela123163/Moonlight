#pragma once


template<typename T>
class ResourceContainer
{
public:
    bool Contains(size_t key) const
    {
        return mResource.find(key) != mResource.end();
    }

    T* Get(size_t key)
    {
        return mResource[key].get();
    }

    void Insert(size_t key, std::unique_ptr<T>&& resource)
    {
        mResource[key] = std::move(resource);
    }

    size_t Size() const
    {
        return mResource.size();
    }

private:
    std::unordered_map<size_t, std::unique_ptr<T>> mResource;
};

