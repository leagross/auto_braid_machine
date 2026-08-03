import { describe, it, expect, vi, beforeEach } from "vitest";
import { api, toSortMs } from "./api";

describe("toSortMs", () => {
  it("returns 0 for null/undefined", () => {
    expect(toSortMs(null)).toBe(0);
    expect(toSortMs(undefined)).toBe(0);
  });

  it("passes through a plain number (createdAt from the ESP32)", () => {
    expect(toSortMs(123456)).toBe(123456);
  });

  it("converts a Firestore Timestamp ({ seconds }) to milliseconds", () => {
    expect(toSortMs({ seconds: 42 })).toBe(42000);
  });

  it("returns 0 for an object with no seconds field", () => {
    expect(toSortMs({})).toBe(0);
  });
});

describe("api (talks to the second ESP32's HTTP server, not Firebase directly)", () => {
  beforeEach(() => {
    vi.stubGlobal("fetch", vi.fn());
  });

  it("login() posts credentials and returns the parsed response", async () => {
    fetch.mockResolvedValueOnce({
      ok: true,
      json: async () => ({ uid: "u1", name: "Dana", role: "user" }),
    });

    const result = await api.login("dana@example.com", "secret");

    expect(fetch).toHaveBeenCalledWith(
      expect.stringContaining("/login"),
      expect.objectContaining({ method: "POST" })
    );
    expect(result).toEqual({ uid: "u1", name: "Dana", role: "user" });
  });

  it("login() throws with the server's error message on failure", async () => {
    fetch.mockResolvedValueOnce({
      ok: false,
      json: async () => ({ message: "wrong password" }),
    });

    await expect(api.login("dana@example.com", "bad")).rejects.toThrow("wrong password");
  });

  it("getMyOrders() sorts orders newest first", async () => {
    fetch.mockResolvedValueOnce({
      ok: true,
      json: async () => [
        { id: "a", createdAt: 1000 },
        { id: "b", createdAt: 3000 },
        { id: "c", createdAt: 2000 },
      ],
    });

    const orders = await api.getMyOrders("u1");

    expect(orders.map((o) => o.id)).toEqual(["b", "c", "a"]);
    expect(fetch).toHaveBeenCalledWith(expect.stringContaining("/my-orders?uid=u1"));
  });

  it("generateCode() returns just the code string", async () => {
    fetch.mockResolvedValueOnce({ ok: true, json: async () => ({ code: "4821" }) });

    const code = await api.generateCode("u1", "Dana");

    expect(code).toBe("4821");
  });
});
